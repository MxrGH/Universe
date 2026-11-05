#!/usr/bin/env python3
"""Generate a Geometry Dash level design draft.

Online mode uses an OpenAI-compatible Chat Completions endpoint.
Offline mode creates a deterministic template draft. The output is a design
blueprint JSON file, not a native Geometry Dash level file.
"""

from __future__ import annotations

import argparse
import json
import os
import random
import sys
import urllib.error
import urllib.request
from pathlib import Path

ALLOWED_OBJECTS = {"block", "spike", "saw", "orb", "pad", "deco"}


def offline_draft(prompt: str, difficulty: str, length: int, style: str, seed: int) -> dict:
    rng = random.Random(seed)
    section_count = max(3, min(24, length // 8))
    spacing = 120
    x = 0
    sections = []
    difficulty_scale = {"easy": 1, "normal": 2, "hard": 3, "insane": 4, "demon": 5}[difficulty]
    for section_index in range(section_count):
        events = []
        event_count = max(2, min(10, difficulty_scale + rng.randint(1, 4)))
        for _ in range(event_count):
            kind = rng.choices(
                ["block", "spike", "saw", "orb", "pad", "deco"],
                weights=[28, 30 + difficulty_scale * 5, 8 + difficulty_scale * 3, 12, 10, 5],
                k=1,
            )[0]
            events.append({
                "x": x,
                "y": rng.choice([0, 30, 60, 90, 120]),
                "type": kind,
                "note": f"{style} section {section_index + 1}",
            })
            x += spacing
        sections.append({
            "name": f"Section {section_index + 1}",
            "gameplay": style,
            "events": events,
        })
        x += spacing * 2

    return {
        "format": "universe-level-draft-v1",
        "generator": "offline-template",
        "title": prompt[:64] or "Untitled Universe Draft",
        "prompt": prompt,
        "difficulty": difficulty,
        "length_units": length,
        "style": style,
        "seed": seed,
        "sections": sections,
        "notes": [
            "This is a design blueprint, not a native Geometry Dash level.",
            "Review spacing, gameplay transitions, and song timing in an editor.",
        ],
    }


def validate_draft(data: object) -> dict:
    if not isinstance(data, dict):
        raise ValueError("The model response must be a JSON object.")
    if not isinstance(data.get("sections"), list) or not data["sections"]:
        raise ValueError("The draft must contain at least one section.")
    total = 0
    for section in data["sections"]:
        if not isinstance(section, dict) or not isinstance(section.get("events"), list):
            raise ValueError("Each section must contain an events array.")
        for event in section["events"]:
            if not isinstance(event, dict):
                raise ValueError("Every event must be an object.")
            if event.get("type") not in ALLOWED_OBJECTS:
                raise ValueError(f"Unsupported object type: {event.get('type')!r}")
            for axis in ("x", "y"):
                if not isinstance(event.get(axis), (int, float)):
                    raise ValueError(f"Every event needs a numeric {axis} coordinate.")
            total += 1
            if total > 5000:
                raise ValueError("Draft is too large (limit: 5000 events).")
    data["format"] = "universe-level-draft-v1"
    return data


def request_ai_draft(args: argparse.Namespace) -> dict:
    api_key = os.environ.get(args.api_key_env)
    if not api_key:
        raise RuntimeError(
            f"{args.api_key_env} is not set. Set it in your local environment, "
            "or use --offline for a deterministic template draft."
        )

    schema = {
        "title": "string",
        "difficulty": "easy|normal|hard|insane|demon",
        "style": "string",
        "sections": [{
            "name": "string",
            "gameplay": "cube|ship|ball|ufo|wave|robot|spider|swing",
            "events": [{
                "x": "number",
                "y": "number",
                "type": "block|spike|saw|orb|pad|deco",
                "note": "short string",
            }],
        }],
    }
    prompt = (
        "Create a coherent Geometry Dash level design blueprint. "
        "Return only JSON matching this shape: " + json.dumps(schema) +
        f". User concept: {args.prompt}. Difficulty: {args.difficulty}. "
        f"Length target: {args.length} units. Gameplay style: {args.style}. "
        "Use increasing x coordinates, varied but playable spacing, and clear "
        "section transitions. Keep the event count below 5000."
    )
    base = args.base_url.rstrip("/")
    payload = {
        "model": args.model,
        "temperature": 0.7,
        "messages": [
            {"role": "system", "content": "You design Geometry Dash levels. Output valid JSON only."},
            {"role": "user", "content": prompt},
        ],
        "response_format": {"type": "json_object"},
    }
    request = urllib.request.Request(
        base + "/chat/completions",
        data=json.dumps(payload).encode("utf-8"),
        headers={
            "Authorization": "Bearer " + api_key,
            "Content-Type": "application/json",
        },
        method="POST",
    )
    try:
        with urllib.request.urlopen(request, timeout=args.timeout) as response:
            result = json.loads(response.read().decode("utf-8"))
    except (urllib.error.URLError, TimeoutError) as error:
        raise RuntimeError(f"AI request failed: {error}") from error

    content = result["choices"][0]["message"]["content"]
    return validate_draft(json.loads(content))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prompt", required=True, help="Describe the level concept.")
    parser.add_argument("--difficulty", choices=["easy", "normal", "hard", "insane", "demon"], default="hard")
    parser.add_argument("--length", type=int, default=60, help="Approximate length in design units.")
    parser.add_argument("--style", default="mixed", help="Preferred gameplay style.")
    parser.add_argument("--seed", type=int, default=2026, help="Offline generator seed.")
    parser.add_argument("--offline", action="store_true", help="Use the built-in deterministic template generator.")
    parser.add_argument("--base-url", default="https://api.openai.com/v1", help="OpenAI-compatible API base URL.")
    parser.add_argument("--model", default="gpt-4o-mini", help="Model name accepted by the API endpoint.")
    parser.add_argument("--api-key-env", default="UNIVERSE_AI_API_KEY", help="Environment variable containing the API key.")
    parser.add_argument("--timeout", type=int, default=60)
    parser.add_argument("--output", type=Path, default=Path("Universe-Level-Draft.json"))
    args = parser.parse_args()

    if args.length < 8 or args.length > 500:
        parser.error("--length must be between 8 and 500.")

    try:
        draft = offline_draft(args.prompt, args.difficulty, args.length, args.style, args.seed) if args.offline else request_ai_draft(args)
        draft = validate_draft(draft)
        args.output.write_text(json.dumps(draft, indent=2) + "\n", encoding="utf-8")
    except (RuntimeError, ValueError, KeyError, IndexError, json.JSONDecodeError) as error:
        print(f"Universe Level Maker: {error}", file=sys.stderr)
        return 1

    count = sum(len(section["events"]) for section in draft["sections"])
    print(f"Saved {len(draft['sections'])} sections and {count} events to {args.output}")
    print("This file is a design blueprint, not an import-ready Geometry Dash level.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
