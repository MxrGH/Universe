#!/usr/bin/env python3
"""Quality gate for Uni-1 JSONL training corpora.

Synthetic bootstrap data is accepted only with an explicit opt-in. Production
training requires rows marked reviewed=true, in addition to structural checks.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

try:
    from .draft_validation import validate_generated_level
except ImportError:
    from draft_validation import validate_generated_level


def validate_dataset(path: Path, *, minimum: int = 32, allow_synthetic: bool = False) -> dict:
    rows = []
    prompts, responses, styles, difficulties = set(), set(), set(), set()
    synthetic_count = 0
    unreviewed_count = 0

    for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not line.strip():
            continue
        try:
            row = json.loads(line)
        except json.JSONDecodeError as error:
            raise ValueError(f"Line {line_number}: invalid JSON: {error.msg}") from error
        if not isinstance(row, dict):
            raise ValueError(f"Line {line_number}: each row must be an object.")
        prompt, response_text = row.get("prompt"), row.get("response")
        if not isinstance(prompt, str) or not prompt.strip() or len(prompt) > 4000:
            raise ValueError(f"Line {line_number}: prompt must be non-empty text of at most 4000 characters.")
        if not isinstance(response_text, str):
            raise ValueError(f"Line {line_number}: response must be a JSON string.")
        try:
            response, object_count = validate_generated_level(json.loads(response_text))
        except (json.JSONDecodeError, ValueError) as error:
            raise ValueError(f"Line {line_number}: invalid generated level: {error}") from error

        is_synthetic = bool(row.get("synthetic")) or any(
            "synthetic bootstrap" in note.lower()
            for note in response.get("notes", [])
            if isinstance(note, str)
        )
        if is_synthetic:
            synthetic_count += 1
            if not allow_synthetic:
                raise ValueError(
                    f"Line {line_number}: synthetic examples are blocked for normal training. "
                    "Use reviewed human-created examples, or explicitly pass --allow-synthetic-bootstrap for experiments."
                )
        elif row.get("reviewed") is not True:
            unreviewed_count += 1
            raise ValueError(
                f"Line {line_number}: production examples must include reviewed=true after human review and playtesting."
            )

        prompts.add(prompt.strip())
        responses.add(json.dumps(response, sort_keys=True, ensure_ascii=False))
        styles.add(response.get("style", ""))
        difficulties.add(response.get("difficulty", ""))
        rows.append((prompt.strip(), object_count))

    if len(rows) < minimum:
        raise ValueError(f"Dataset has {len(rows)} examples; at least {minimum} are required.")
    if len(prompts) / len(rows) < 0.9:
        raise ValueError("At least 90% of prompts must be unique.")
    if len(responses) / len(rows) < 0.9:
        raise ValueError("At least 90% of level responses must be unique.")
    if len(styles) < 4 or len(difficulties) < 3:
        raise ValueError("Dataset needs at least 4 styles and 3 difficulty classes.")

    return {
        "examples": len(rows),
        "unique_prompts": len(prompts),
        "unique_responses": len(responses),
        "styles": len(styles),
        "difficulties": len(difficulties),
        "synthetic_examples": synthetic_count,
        "reviewed_examples": len(rows) - synthetic_count - unreviewed_count,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dataset", type=Path)
    parser.add_argument("--minimum", type=int, default=32)
    parser.add_argument("--allow-synthetic-bootstrap", action="store_true")
    args = parser.parse_args()
    if args.minimum < 2:
        parser.error("--minimum must be at least 2.")
    if not args.dataset.is_file():
        parser.error(f"Dataset not found: {args.dataset}")
    try:
        report = validate_dataset(
            args.dataset,
            minimum=args.minimum,
            allow_synthetic=args.allow_synthetic_bootstrap,
        )
    except ValueError as error:
        parser.error(str(error))
    print(json.dumps(report, indent=2, sort_keys=True))
    print("PASS: dataset structure and diversity checks passed.")
    if report["synthetic_examples"]:
        print("WARNING: synthetic bootstrap data is not evidence of playable level quality.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
