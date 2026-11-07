#!/usr/bin/env python3
"""Generate, validate, and export Geometry Dash level-design blueprints.

The JSON format is an editor-facing design blueprint. It is not a native
Geometry Dash .gmd save and cannot be loaded directly into the game.
"""

from __future__ import annotations

import argparse
import csv
import json
import math
import os
import random
import re
import sys
import urllib.error
import urllib.request
from pathlib import Path

ALLOWED_OBJECTS = {"block", "spike", "saw", "orb", "pad", "deco"}
ALLOWED_GAMEPLAY = {"cube", "ship", "ball", "ufo", "wave", "robot", "spider", "swing", "jetpack"}
DIFFICULTIES = {"easy", "normal", "hard", "insane", "demon"}
MAX_SECTIONS = 100
MAX_EVENTS = 5000
MAX_COORDINATE = 10_000_000


def validate_draft(data: object) -> dict:
    if not isinstance(data, dict):
        raise ValueError("The draft must be a JSON object.")
    sections = data.get("sections")
    if not isinstance(sections, list) or not sections:
        raise ValueError("The draft must contain at least one section.")
    if len(sections) > MAX_SECTIONS:
        raise ValueError(f"Draft has too many sections (limit: {MAX_SECTIONS}).")

    for field, default, limit in (
        ("title", "Untitled Universe Draft", 120),
        ("prompt", "", 4000),
        ("difficulty", "hard", 16),
        ("style", "mixed", 80),
    ):
        value = data.get(field, default)
        if not isinstance(value, str) or len(value) > limit:
            raise ValueError(f"Draft field {field!r} must be text of at most {limit} characters.")
        data[field] = value
    if not isinstance(data["difficulty"], str) or data["difficulty"] not in DIFFICULTIES:
        raise ValueError("Difficulty must be easy, normal, hard, insane, or demon.")

    total = 0
    previous_x = float("-inf")
    for section_index, section in enumerate(sections):
        if not isinstance(section, dict):
            raise ValueError("Every section must be an object.")
        name = section.get("name", f"Section {section_index + 1}")
        gameplay = section.get("gameplay", "cube")
        events = section.get("events")
        if not isinstance(name, str) or len(name) > 120:
            raise ValueError("Each section name must be text of at most 120 characters.")
        if not isinstance(gameplay, str) or gameplay not in ALLOWED_GAMEPLAY:
            raise ValueError(f"Unsupported gameplay mode: {gameplay!r}")
        if not isinstance(events, list) or not events:
            raise ValueError("Every section must contain at least one event.")
        section["name"] = name
        section["gameplay"] = gameplay
        for event in events:
            if not isinstance(event, dict):
                raise ValueError("Every event must be an object.")
            if not isinstance(event.get("type"), str) or event["type"] not in ALLOWED_OBJECTS:
                raise ValueError(f"Unsupported object type: {event.get('type')!r}")
            for axis in ("x", "y"):
                coordinate = event.get(axis)
                if isinstance(coordinate, bool) or not isinstance(coordinate, (int, float)):
                    raise ValueError(f"Every event needs a numeric {axis} coordinate.")
                if not math.isfinite(coordinate):
                    raise ValueError(f"Every event needs a finite {axis} coordinate.")
                if abs(coordinate) > MAX_COORDINATE:
                    raise ValueError(f"Event {axis} coordinate is outside the supported range.")
            if event["x"] < previous_x:
                raise ValueError("Event x coordinates must be in non-decreasing order.")
            previous_x = event["x"]
            note = event.get("note", "")
            if not isinstance(note, str) or len(note) > 240:
                raise ValueError("Event notes must be text of at most 240 characters.")
            total += 1
            if total > MAX_EVENTS:
                raise ValueError(f"Draft is too large (limit: {MAX_EVENTS} events).")

    notes = data.get("notes", [])
    if not isinstance(notes, list) or len(notes) > 100:
        raise ValueError("Draft notes must be a list containing at most 100 items.")
    if any(not isinstance(note, str) or len(note) > 500 for note in notes):
        raise ValueError("Each draft note must be text of at most 500 characters.")
    data["format"] = "universe-level-draft-v1"
    return data


def strip_json_fence(content: str) -> str:
    text = content.strip()
    if text.startswith("```"):
        text = re.sub(r"^\`\`\`(?:json)?\s*", "", text, count=1, flags=re.IGNORECASE)
        text = re.sub(r"\s*\`\`\`$", "", text, count=1)
    return text.strip()


def request_ai_draft(args: argparse.Namespace) -> dict:
    key_env = args.api_key_env or ("ANTHROPIC_API_KEY" if args.provider == "anthropic" else "UNIVERSE_AI_API_KEY")
    api_key = os.environ.get(key_env)
    if not api_key:
        raise RuntimeError(f"{key_env} is not set. Add it to your local environment; never commit it.")

    schema = {
        "title": "string",
        "prompt": "string",
        "difficulty": "easy|normal|hard|insane|demon",
        "style": "string",
        "sections": [{
            "name": "string",
            "gameplay": "cube|ship|ball|ufo|wave|robot|spider|swing|jetpack",
            "events": [{"x": "number", "y": "number", "type": "block|spike|saw|orb|pad|deco", "note": "short string"}],
        }],
        "notes": ["string"],
    }
    prompt = (
        "Create a coherent Geometry Dash level design blueprint. Return only a JSON object matching this schema: "
        + json.dumps(schema) + f". User concept: {args.prompt}. Difficulty: {args.difficulty}. "
        + f"Length target: {args.length} design units. Preferred style: {args.style}. "
        + "Use non-decreasing x coordinates, finite coordinates, playable spacing, and clear section transitions. "
        + "Keep at most 100 sections and 5000 events. Do not wrap the JSON in Markdown fences. "
        + "For song/Newgrounds ideas, provide mood/search suggestions only; do not claim to download or license music. "
        + "This is a design blueprint, not a playable Geometry Dash level save."
    )
    model = args.model or ("claude-sonnet-4-6" if args.provider == "anthropic" else "gpt-4o-mini")
    if args.provider == "anthropic":
        base = (args.base_url or "https://api.anthropic.com").rstrip("/")
        if not base.startswith("https://"):
            raise RuntimeError("AI API base URL must use HTTPS so the API key is not sent over an unencrypted connection.")
        endpoint = base + "/v1/messages"
        payload = {
            "model": model,
            "max_tokens": 8192,
            "system": "You design Geometry Dash levels. Output valid JSON only, without Markdown fences.",
            "messages": [{"role": "user", "content": prompt}],
        }
        headers = {"x-api-key": api_key, "anthropic-version": "2023-06-01", "Content-Type": "application/json"}
    else:
        base = (args.base_url or "https://api.openai.com/v1").rstrip("/")
        if not base.startswith("https://"):
            raise RuntimeError("AI API base URL must use HTTPS so the API key is not sent over an unencrypted connection.")
        endpoint = base + "/chat/completions"
        payload = {
            "model": model,
            "temperature": 0.7,
            "messages": [
                {"role": "system", "content": "You design Geometry Dash levels. Output valid JSON only."},
                {"role": "user", "content": prompt},
            ],
            "response_format": {"type": "json_object"},
        }
        headers = {"Authorization": "Bearer " + api_key, "Content-Type": "application/json"}

    request = urllib.request.Request(endpoint, data=json.dumps(payload).encode("utf-8"), headers=headers, method="POST")
    try:
        with urllib.request.urlopen(request, timeout=args.timeout) as response:
            raw = response.read(2_000_001)
            if len(raw) > 2_000_000:
                raise RuntimeError("AI response is too large (limit: 2 MB).")
            result = json.loads(raw.decode("utf-8"))
    except urllib.error.HTTPError as error:
        details = error.read(1000).decode("utf-8", errors="replace")
        raise RuntimeError(f"AI request failed (HTTP {error.code}): {details}") from error
    except (urllib.error.URLError, TimeoutError, UnicodeDecodeError, json.JSONDecodeError) as error:
        raise RuntimeError(f"AI request failed: {error}") from error

    try:
        if args.provider == "anthropic":
            content = "".join(block.get("text", "") for block in result["content"] if block.get("type") == "text")
        else:
            content = result["choices"][0]["message"]["content"]
        return validate_draft(json.loads(strip_json_fence(content)))
    except (KeyError, IndexError, TypeError, json.JSONDecodeError) as error:
        raise RuntimeError(f"Could not parse the AI response: {error}") from error


def export_draft(data: dict, output: Path, output_format: str) -> None:
    output.parent.mkdir(parents=True, exist_ok=True)
    if output_format == "json":
        output.write_text(json.dumps(data, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    elif output_format == "csv":
        with output.open("w", encoding="utf-8-sig", newline="") as stream:
            writer = csv.writer(stream)
            writer.writerow(["section", "gameplay", "x", "y", "object_type", "note"])
            for section in data["sections"]:
                for event in section["events"]:
                    writer.writerow([
                        section["name"], section["gameplay"], event["x"], event["y"],
                        event["type"], event.get("note", ""),
                    ])
    else:
        lines = [
            f"# {data['title']}",
            "",
            f"- Difficulty: {data['difficulty']}",
            f"- Style: {data['style']}",
            f"- Prompt: {data['prompt']}",
            "",
        ]
        for section in data["sections"]:
            lines.extend([f"## {section['name']} — {section['gameplay']}", "",
                          "| X | Y | Object | Note |", "|---:|---:|---|---|"])
            for event in section["events"]:
                note = event.get("note", "").replace("|", "\\|").replace("\n", " ")
                lines.append(f"| {event['x']} | {event['y']} | {event['type']} | {note} |")
            lines.append("")
        if data.get("notes"):
            lines.extend(["## Design notes", ""])
            lines.extend(f"- {note}" for note in data["notes"])
        output.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--prompt", help="Describe a new level concept.")
    source.add_argument("--input", type=Path, help="Load and validate an existing Universe JSON blueprint.")
    parser.add_argument("--difficulty", choices=sorted(DIFFICULTIES), default="hard")
    parser.add_argument("--length", type=int, default=60, help="Approximate length in design units.")
    parser.add_argument("--style", default="mixed", help="Preferred gameplay style.")
    parser.add_argument("--provider", choices=["anthropic", "openai-compatible"], default="anthropic")
    parser.add_argument("--base-url", default=None, help="Optional API base URL override.")
    parser.add_argument("--model", default=None, help="Model name override.")
    parser.add_argument("--api-key-env", default=None, help="Environment variable containing the API key.")
    parser.add_argument("--timeout", type=int, default=60)
    parser.add_argument("--format", choices=["json", "csv", "markdown"], default="json",
                        help="Output format. Use --input to convert an existing JSON blueprint.")
    parser.add_argument("--output", type=Path, default=Path("Universe-Level-Draft.json"))
    args = parser.parse_args()

    if not 1 <= args.timeout <= 300:
        parser.error("--timeout must be between 1 and 300.")
    if args.length < 8 or args.length > 500:
        parser.error("--length must be between 8 and 500.")

    try:
        if args.input:
            if args.input.stat().st_size > 5_000_000:
                raise ValueError("Input blueprint is too large (limit: 5 MB).")
            raw_input = args.input.read_text(encoding="utf-8")
            data = json.loads(strip_json_fence(raw_input))
            draft = validate_draft(data)
        else:
            draft = validate_draft(request_ai_draft(args))
        export_draft(draft, args.output, args.format)
    except (RuntimeError, ValueError, KeyError, IndexError, OSError, OverflowError, json.JSONDecodeError) as error:
        print(f"Universe Level Maker: {error}", file=sys.stderr)
        return 1

    count = sum(len(section["events"]) for section in draft["sections"])
    print(f"Saved {len(draft['sections'])} sections and {count} events to {args.output} ({args.format})")
    if args.format == "json":
        print("This file is a design blueprint, not an import-ready Geometry Dash level.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
