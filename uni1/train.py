#!/usr/bin/env python3
"""Fine-tune the Uni-1 Geometry Dash specialist adapter with a held-out eval set."""

from __future__ import annotations

import argparse
import json
import random
from pathlib import Path

import torch
from peft import LoraConfig, get_peft_model
from torch.utils.data import Dataset, random_split
from transformers import (
    AutoModelForCausalLM,
    AutoTokenizer,
    Trainer,
    TrainingArguments,
)

try:
    from .draft_validation import validate_draft_payload
except ImportError:  # direct execution: python train.py
    from draft_validation import validate_draft_payload

DEFAULT_BASE = "Qwen/Qwen2.5-0.5B-Instruct"
SYSTEM_PROMPT = (
    "You are Uni-1, a Geometry Dash level-design assistant for Universe. "
    "Return only one valid JSON object using the Universe level-draft schema. "
    "Use only block, spike, saw, orb, pad, and deco objects. "
    "Create a coherent, readable cube-mode layout with fair spacing and a clear route. "
    "Do not claim the layout was playtested."
)


class BlueprintDataset(Dataset):
    """Tokenize validated examples while masking prompt tokens from the loss."""

    def __init__(self, path: Path, tokenizer, max_length: int):
        self.rows = []
        for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            if not line.strip():
                continue
            try:
                row = json.loads(line)
            except json.JSONDecodeError as error:
                raise ValueError(f"Line {line_number} is not valid JSON: {error.msg}") from error
            if (
                not isinstance(row, dict)
                or not isinstance(row.get("prompt"), str)
                or not row["prompt"].strip()
                or len(row["prompt"]) > 4000
                or not isinstance(row.get("response"), str)
            ):
                raise ValueError(
                    f"Line {line_number} must contain a non-empty prompt (max 4000 chars) "
                    "and a JSON response string."
                )
            try:
                response = json.loads(row["response"])
                response, object_count = validate_draft_payload(response)
            except (json.JSONDecodeError, ValueError) as error:
                raise ValueError(f"Line {line_number} has an invalid Universe draft: {error}") from error
            if object_count == 0:
                raise ValueError(f"Line {line_number} contains no trainable object placements.")

            # Train on the normalized schema, not the original unnormalized string.
            response_text = json.dumps(response, ensure_ascii=False, separators=(",", ":"))
            messages = [
                {"role": "system", "content": SYSTEM_PROMPT},
                {"role": "user", "content": row["prompt"].strip()},
            ]
            prefix_ids = tokenizer.apply_chat_template(
                messages,
                tokenize=True,
                add_generation_prompt=True,
            )
            full_ids = tokenizer.apply_chat_template(
                messages + [{"role": "assistant", "content": response_text}],
                tokenize=True,
                add_generation_prompt=False,
            )
            if not isinstance(prefix_ids, list) or not isinstance(full_ids, list):
                raise ValueError("Tokenizer chat template did not return token ID lists.")
            if len(full_ids) <= len(prefix_ids):
                raise ValueError(
                    f"Line {line_number} has no assistant response tokens after chat-template encoding."
                )

            input_ids = full_ids[:max_length]
            trainable_start = min(len(prefix_ids), len(input_ids))
            if trainable_start >= len(input_ids):
                raise ValueError(
                    f"Line {line_number} is too long: truncation removed the entire assistant response. "
                    "Increase --max-length or shorten the example."
                )
            labels = list(input_ids)
            labels[:trainable_start] = [-100] * trainable_start
            self.rows.append({
                "input_ids": input_ids,
                "attention_mask": [1] * len(input_ids),
                "labels": labels,
            })

        if len(self.rows) < 2:
            raise ValueError("Training needs at least two valid examples for a train/evaluation split.")

    def __len__(self):
        return len(self.rows)

    def __getitem__(self, index):
        return self.rows[index]


class CausalPadCollator:
    """Right-pad causal-LM batches and exclude padding from the loss."""

    def __init__(self, tokenizer):
        if tokenizer.pad_token_id is None:
            raise ValueError("Tokenizer must have a pad token before training.")
        self.pad_token_id = tokenizer.pad_token_id

    def __call__(self, features):
        max_len = max(len(item["input_ids"]) for item in features)
        input_ids, attention, labels = [], [], []
        for item in features:
            amount = max_len - len(item["input_ids"])
            input_ids.append(item["input_ids"] + [self.pad_token_id] * amount)
            attention.append(item["attention_mask"] + [0] * amount)
            labels.append(item["labels"] + [-100] * amount)
        return {
            "input_ids": torch.tensor(input_ids, dtype=torch.long),
            "attention_mask": torch.tensor(attention, dtype=torch.long),
            "labels": torch.tensor(labels, dtype=torch.long),
        }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dataset", type=Path, default=Path("dataset/train.jsonl"))
    parser.add_argument("--base-model", default=DEFAULT_BASE)
    parser.add_argument("--output", type=Path, default=Path("artifacts/uni1-adapter"))
    parser.add_argument("--epochs", type=float, default=3.0)
    parser.add_argument("--max-length", type=int, default=8192)
    parser.add_argument("--learning-rate", type=float, default=2e-4)
    parser.add_argument("--validation-ratio", type=float, default=0.1)
    parser.add_argument("--seed", type=int, default=20261107)
    parser.add_argument("--max-steps", type=int, default=-1)
    args = parser.parse_args()

    if not args.dataset.is_file():
        raise SystemExit(f"Dataset not found: {args.dataset}")
    if not 256 <= args.max_length <= 16384:
        raise SystemExit("--max-length must be between 256 and 16384.")
    if args.epochs <= 0 or args.learning_rate <= 0:
        raise SystemExit("--epochs and --learning-rate must be positive.")
    if not 0.05 <= args.validation_ratio <= 0.5:
        raise SystemExit("--validation-ratio must be between 0.05 and 0.5.")
    if args.max_steps == 0 or args.max_steps < -1:
        raise SystemExit("--max-steps must be -1 (disabled) or a positive integer.")

    random.seed(args.seed)
    torch.manual_seed(args.seed)
    if torch.cuda.is_available():
        torch.cuda.manual_seed_all(args.seed)
        dtype = torch.bfloat16 if torch.cuda.is_bf16_supported() else torch.float16
    else:
        dtype = torch.float32
        print("WARNING: CUDA is unavailable. CPU fine-tuning will be very slow.")

    tokenizer = AutoTokenizer.from_pretrained(args.base_model, use_fast=True)
    if tokenizer.pad_token_id is None:
        if tokenizer.eos_token is None:
            raise SystemExit("The base tokenizer has neither a pad token nor an EOS token.")
        tokenizer.pad_token = tokenizer.eos_token

    # Validate and tokenize the complete corpus before downloading model weights.
    dataset = BlueprintDataset(args.dataset, tokenizer, args.max_length)
    validation_size = max(1, round(len(dataset) * args.validation_ratio))
    training_size = len(dataset) - validation_size
    if training_size < 1:
        raise SystemExit("Dataset split left no training examples.")
    train_dataset, eval_dataset = random_split(
        dataset,
        [training_size, validation_size],
        generator=torch.Generator().manual_seed(args.seed),
    )

    model = AutoModelForCausalLM.from_pretrained(
        args.base_model,
        torch_dtype=dtype,
    )
    model.config.use_cache = False
    model.config.pad_token_id = tokenizer.pad_token_id
    model.gradient_checkpointing_enable()
    model.enable_input_require_grads()
    model = get_peft_model(
        model,
        LoraConfig(
            r=16,
            lora_alpha=32,
            lora_dropout=0.05,
            bias="none",
            task_type="CAUSAL_LM",
            target_modules=[
                "q_proj", "k_proj", "v_proj", "o_proj",
                "gate_proj", "up_proj", "down_proj",
            ],
        ),
    )
    model.print_trainable_parameters()

    training_args = TrainingArguments(
        output_dir=str(args.output),
        num_train_epochs=args.epochs,
        max_steps=args.max_steps,
        learning_rate=args.learning_rate,
        per_device_train_batch_size=1,
        per_device_eval_batch_size=1,
        gradient_accumulation_steps=8,
        logging_steps=5,
        evaluation_strategy="epoch",
        save_strategy="epoch",
        save_total_limit=2,
        load_best_model_at_end=True,
        metric_for_best_model="eval_loss",
        greater_is_better=False,
        bf16=dtype == torch.bfloat16,
        fp16=dtype == torch.float16,
        gradient_checkpointing=True,
        optim="adamw_torch",
        report_to="none",
        remove_unused_columns=False,
        seed=args.seed,
        data_seed=args.seed,
    )
    trainer = Trainer(
        model=model,
        args=training_args,
        train_dataset=train_dataset,
        eval_dataset=eval_dataset,
        data_collator=CausalPadCollator(tokenizer),
    )
    trainer.train()
    metrics = trainer.evaluate()
    args.output.mkdir(parents=True, exist_ok=True)
    model.save_pretrained(args.output)
    tokenizer.save_pretrained(args.output)
    print(f"Saved Uni-1 LoRA adapter to {args.output}")
    print(f"Held-out evaluation metrics: {json.dumps(metrics, sort_keys=True)}")
    print("This is a trained adapter checkpoint, not a standalone merged model.")
    print("Evaluate generated drafts and playtest them before calling this a usable release.")


if __name__ == "__main__":
    main()
