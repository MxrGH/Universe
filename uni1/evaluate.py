#!/usr/bin/env python3
"""Generate held-out Uni-1 outputs and measure real schema/feature coverage.

The evaluation prompts must be a separate, human-reviewed JSONL file. This
script measures structural validity and feature coverage; it does not simulate
Geometry Dash physics or prove that generated levels are playable.
"""
from __future__ import annotations

import argparse
import json
from collections import Counter
from pathlib import Path


try:
    from .draft_validation import validate_generated_level
    from .object_catalog import OBJECT_IDS
    from .server import SYSTEM_PROMPT
except ImportError:
    from draft_validation import validate_generated_level
    from object_catalog import OBJECT_IDS
    from server import SYSTEM_PROMPT


DEFAULT_BASE = "Qwen/Qwen2.5-0.5B-Instruct"


def load_eval_prompts(path: Path, *, limit: int | None = None) -> list[str]:
    """Load a separate, reviewed prompt set; reject accidental training fixtures."""
    prompts: list[str] = []
    seen: set[str] = set()
    for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not line.strip():
            continue
        try:
            row = json.loads(line)
        except json.JSONDecodeError as error:
            raise ValueError(f"Line {line_number}: invalid JSON: {error.msg}") from error
        if not isinstance(row, dict):
            raise ValueError(f"Line {line_number}: expected a JSON object.")
        prompt = row.get("prompt")
        if not isinstance(prompt, str) or not prompt.strip() or len(prompt) > 4000:
            raise ValueError(f"Line {line_number}: prompt must be non-empty text up to 4000 characters.")
        if row.get("reviewed") is not True:
            raise ValueError(f"Line {line_number}: evaluation prompts must include reviewed=true.")
        prompt = prompt.strip()
        if prompt in seen:
            raise ValueError(f"Line {line_number}: duplicate evaluation prompt.")
        seen.add(prompt)
        prompts.append(prompt)
        if limit is not None and len(prompts) >= limit:
            break
    if not prompts:
        raise ValueError("The evaluation prompt file contains no reviewed prompts.")
    return prompts


def summarize_results(results: list[dict]) -> dict:
    """Aggregate validation and editor-feature coverage from generated drafts."""
    total = len(results)
    valid = [item for item in results if item.get("valid") is True]
    modes: Counter[str] = Counter()
    object_types: Counter[str] = Counter()
    object_families: Counter[str] = Counter()
    placements = []
    sections = []
    for item in valid:
        draft = item["draft"]
        sections.append(len(draft["sections"]))
        count = 0
        for section in draft["sections"]:
            modes[section["gameplay"]] += 1
            for event in section["events"]:
                count += 1
                kind = event["type"]
                object_types[kind] += 1
                if kind == "object":
                    name = next((name for name, object_id in OBJECT_IDS.items()
                                 if object_id == event.get("id")), "unknown")
                    family = name.split("_", 1)[0]
                    object_families[family] += 1
                else:
                    object_families["primitive_" + kind] += 1
        placements.append(count)
    return {
        "total_prompts": total,
        "valid_outputs": len(valid),
        "invalid_outputs": total - len(valid),
        "valid_rate": round(len(valid) / total, 4) if total else 0.0,
        "average_sections": round(sum(sections) / len(sections), 2) if sections else 0.0,
        "average_placements": round(sum(placements) / len(placements), 2) if placements else 0.0,
        "gameplay_modes": dict(sorted(modes.items())),
        "object_types": dict(sorted(object_types.items())),
        "object_families": dict(sorted(object_families.items())),
        "physics_playtested": False,
    }


def generate_one(model, tokenizer, prompt: str, *, max_new_tokens: int, temperature: float) -> str:
    messages = [
        {"role": "system", "content": SYSTEM_PROMPT},
        {"role": "user", "content": prompt},
    ]
    encoded = tokenizer.apply_chat_template(
        messages, tokenize=True, add_generation_prompt=True, return_tensors="pt"
    )
    encoded = encoded.to(model.device)
    generation = {
        "max_new_tokens": max_new_tokens,
        "do_sample": temperature > 0,
        "pad_token_id": tokenizer.eos_token_id,
        "eos_token_id": tokenizer.eos_token_id,
    }
    if temperature > 0:
        generation["temperature"] = temperature
    import torch
    with torch.inference_mode():
        output = model.generate(encoded, **generation)
    new_tokens = output[0][encoded.shape[1]:]
    return tokenizer.decode(new_tokens, skip_special_tokens=True).strip()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prompts", type=Path, required=True,
                        help="Separate JSONL evaluation prompts; every row needs reviewed=true.")
    parser.add_argument("--base-model", default=DEFAULT_BASE)
    parser.add_argument("--adapter", type=Path, default=Path("artifacts/uni1-adapter"))
    parser.add_argument("--report", type=Path, default=Path("artifacts/uni1-evaluation.json"))
    parser.add_argument("--max-examples", type=int, default=100)
    parser.add_argument("--max-new-tokens", type=int, default=4096)
    parser.add_argument("--temperature", type=float, default=0.2)
    parser.add_argument("--minimum-valid-rate", type=float, default=0.9)
    parser.add_argument("--seed", type=int, default=20261107)
    args = parser.parse_args()
    if not 1 <= args.max_examples <= 1000:
        parser.error("--max-examples must be between 1 and 1000.")
    if not 64 <= args.max_new_tokens <= 8192:
        parser.error("--max-new-tokens must be between 64 and 8192.")
    if not 0 <= args.temperature <= 1.5:
        parser.error("--temperature must be between 0 and 1.5.")
    if not 0 <= args.minimum_valid_rate <= 1:
        parser.error("--minimum-valid-rate must be between 0 and 1.")
    if not args.adapter.is_dir():
        parser.error(f"Adapter directory not found: {args.adapter}")

    try:
        prompts = load_eval_prompts(args.prompts, limit=args.max_examples)
    except (OSError, ValueError) as error:
        parser.error(str(error))
    import torch
    from peft import PeftModel
    from transformers import AutoModelForCausalLM, AutoTokenizer

    torch.manual_seed(args.seed)
    tokenizer = AutoTokenizer.from_pretrained(args.adapter, use_fast=True)
    if tokenizer.pad_token_id is None:
        if tokenizer.eos_token is None:
            parser.error("Adapter tokenizer has no pad or EOS token.")
        tokenizer.pad_token = tokenizer.eos_token
    dtype = (torch.bfloat16 if torch.cuda.is_available() and torch.cuda.is_bf16_supported()
             else torch.float16 if torch.cuda.is_available() else torch.float32)
    base = AutoModelForCausalLM.from_pretrained(
        args.base_model, torch_dtype=dtype, device_map="auto"
    )
    model = PeftModel.from_pretrained(base, args.adapter)
    model.eval()

    results = []
    for index, prompt in enumerate(prompts, 1):
        try:
            raw = generate_one(
                model, tokenizer, prompt,
                max_new_tokens=args.max_new_tokens,
                temperature=args.temperature,
            )
            draft, count = validate_generated_level(json.loads(raw))
            results.append({
                "index": index, "valid": True, "placements": count,
                "draft": draft,
            })
        except (ValueError, TypeError, KeyError, json.JSONDecodeError, RuntimeError) as error:
            results.append({"index": index, "valid": False, "error": str(error)[:500]})
        print(f"[{index}/{len(prompts)}] {'PASS' if results[-1]['valid'] else 'FAIL'}")

    report = summarize_results(results)
    report.update({
        "base_model": args.base_model,
        "adapter": str(args.adapter),
        "seed": args.seed,
        "max_new_tokens": args.max_new_tokens,
        "temperature": args.temperature,
        "minimum_valid_rate": args.minimum_valid_rate,
        "cases": [{key: value for key, value in item.items() if key != "draft"}
                  for item in results],
    })
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n",
                           encoding="utf-8")
    print(json.dumps(report, indent=2, ensure_ascii=False))
    print(f"Evaluation report saved to {args.report}")
    if report["valid_rate"] < args.minimum_valid_rate:
        print("FAIL: generated-output validity is below the required threshold.")
        return 1
    print("PASS: structural generation threshold met. Playtesting is still required.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
