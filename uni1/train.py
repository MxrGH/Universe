#!/usr/bin/env python3
"""Fine-tune a small instruction model into the Uni-1 Geometry Dash specialist."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

import torch
from peft import LoraConfig, get_peft_model
from torch.utils.data import Dataset
from transformers import AutoModelForCausalLM, AutoTokenizer, Trainer, TrainingArguments

DEFAULT_BASE = "Qwen/Qwen2.5-0.5B-Instruct"
SYSTEM_PROMPT = (
    "You are Uni-1, a Geometry Dash level-design assistant for Universe. "
    "Return only a valid JSON object using the Universe level-draft schema. "
    "Design coherent, playable layouts; use supported object types only: "
    "block, spike, saw, orb, pad, deco. Never claim the result is playtested."
)


class BlueprintDataset(Dataset):
    def __init__(self, path: Path, tokenizer, max_length: int):
        self.rows = []
        for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            if not line.strip():
                continue
            row = json.loads(line)
            if not isinstance(row, dict) or not isinstance(row.get("prompt"), str) or not isinstance(row.get("response"), str):
                raise ValueError(f"Line {line_number} must contain string prompt and response fields.")
            response = json.loads(row["response"])
            try:
                from .draft_validation import validate_draft_payload
            except ImportError:  # direct execution: python train.py
                from draft_validation import validate_draft_payload
            try:
                response, object_count = validate_draft_payload(response)
            except ValueError as error:
                raise ValueError(f"Line {line_number} has an invalid Universe draft: {error}") from error
            if object_count == 0:
                raise ValueError(f"Line {line_number} contains no trainable object placements.")
            for section in response["sections"]:
                for event in section["events"]:
                    event["action"] = "add"
            messages = [
                {"role": "system", "content": SYSTEM_PROMPT},
                {"role": "user", "content": row["prompt"]},
            ]
            prefix = tokenizer.apply_chat_template(messages, tokenize=False, add_generation_prompt=True)
            full = tokenizer.apply_chat_template(
                messages + [{"role": "assistant", "content": row["response"]}],
                tokenize=False,
                add_generation_prompt=False,
            )
            prefix_ids = tokenizer(prefix, add_special_tokens=False)["input_ids"]
            encoded = tokenizer(full, max_length=max_length, truncation=True, add_special_tokens=False)
            input_ids = encoded["input_ids"]
            labels = list(input_ids)
            mask_count = min(len(prefix_ids), len(labels))
            labels[:mask_count] = [-100] * mask_count
            if all(token == -100 for token in labels):
                raise ValueError(f"Line {line_number} is too long or has no trainable response tokens.")
            self.rows.append({
                "input_ids": input_ids,
                "attention_mask": encoded["attention_mask"],
                "labels": labels,
            })
        if not self.rows:
            raise ValueError("Training dataset is empty.")

    def __len__(self):
        return len(self.rows)

    def __getitem__(self, index):
        return self.rows[index]


class CausalPadCollator:
    def __init__(self, tokenizer):
        self.tokenizer = tokenizer

    def __call__(self, features):
        max_len = max(len(item["input_ids"]) for item in features)
        pad_id = self.tokenizer.pad_token_id
        input_ids, attention, labels = [], [], []
        for item in features:
            amount = max_len - len(item["input_ids"])
            input_ids.append(item["input_ids"] + [pad_id] * amount)
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
    parser.add_argument("--max-length", type=int, default=4096)
    parser.add_argument("--learning-rate", type=float, default=2e-4)
    args = parser.parse_args()

    if not args.dataset.is_file():
        raise SystemExit(f"Dataset not found: {args.dataset}")
    if args.max_length < 256 or args.max_length > 8192:
        raise SystemExit("--max-length must be between 256 and 8192.")

    tokenizer = AutoTokenizer.from_pretrained(args.base_model, use_fast=True)
    if tokenizer.pad_token_id is None:
        tokenizer.pad_token = tokenizer.eos_token

    dtype = torch.bfloat16 if torch.cuda.is_available() and torch.cuda.is_bf16_supported() else torch.float32
    model = AutoModelForCausalLM.from_pretrained(
        args.base_model,
        torch_dtype=dtype,
    )
    model.config.use_cache = False
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
            target_modules=["q_proj", "k_proj", "v_proj", "o_proj", "gate_proj", "up_proj", "down_proj"],
        ),
    )
    model.print_trainable_parameters()

    dataset = BlueprintDataset(args.dataset, tokenizer, args.max_length)
    training_args = TrainingArguments(
        output_dir=str(args.output),
        num_train_epochs=args.epochs,
        learning_rate=args.learning_rate,
        per_device_train_batch_size=1,
        gradient_accumulation_steps=8,
        logging_steps=5,
        save_strategy="epoch",
        save_total_limit=2,
        bf16=dtype == torch.bfloat16,
        fp16=False,
        gradient_checkpointing=True,
        optim="adamw_torch",
        report_to="none",
        remove_unused_columns=False,
    )
    trainer = Trainer(
        model=model,
        args=training_args,
        train_dataset=dataset,
        data_collator=CausalPadCollator(tokenizer),
    )
    trainer.train()
    args.output.mkdir(parents=True, exist_ok=True)
    model.save_pretrained(args.output)
    tokenizer.save_pretrained(args.output)
    print(f"Saved Uni-1 LoRA adapter to {args.output}")
    print("This is a trained adapter checkpoint, not a standalone merged model.")


if __name__ == "__main__":
    main()
