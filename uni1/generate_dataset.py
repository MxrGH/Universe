#!/usr/bin/env python3
"""Build a reproducible synthetic bootstrap corpus for Uni-1.

These examples are generated from explicit layout rules. They are useful for
teaching the output schema and simple spacing patterns, not a substitute for
human-authored, playtested Geometry Dash design data.
"""

from __future__ import annotations

import argparse
import json
import random
from pathlib import Path

STYLES = [
    "neon", "industrial", "forest", "space", "retro", "underwater",
    "volcanic", "crystal", "monochrome", "sunset", "digital", "mechanical",
]
DIFFICULTIES = ["easy", "normal", "hard", "insane"]
MODES = ["cube", "ship", "ball", "ufo", "wave", "robot", "spider", "swing"]
OBJECTS = ["block", "spike", "saw", "orb", "pad", "deco"]
SECTION_NAMES = [
    "Opening", "Warm-up", "Main Theme", "Variation", "Challenge", "Finale",
    "Transition", "Climax", "Outro",
]


def make_example(index: int, rng: random.Random) -> dict:
    style = STYLES[index % len(STYLES)]
    difficulty = DIFFICULTIES[(index // 3) % len(DIFFICULTIES)]
    length = rng.randint(6, 8)
    sections = []
    x = 0
    event_count = 0

    for section_index in range(length):
        mode = MODES[(index + section_index * 3) % len(MODES)]
        section_events = []
        section_width = rng.randint(300, 430)
        event_slots = rng.randint(5, 7)
        spacing = section_width / event_slots
        base_y = rng.choice([-30, 0, 30, 60])
        for slot in range(event_slots):
            x += int(spacing)
            if slot == 0:
                object_type = "block"
            elif slot == event_slots - 1:
                object_type = rng.choice(["block", "deco"])
            else:
                choices = ["block", "spike", "orb", "pad", "deco"]
                if difficulty in ("hard", "insane"):
                    choices += ["saw", "saw"]
                object_type = rng.choice(choices)
            y = base_y + rng.choice([-30, 0, 0, 30, 60])
            if object_type == "saw":
                y += 45
            section_events.append({
                "action": "add",
                "x": x,
                "y": y,
                "type": object_type,
                "note": f"{mode} {object_type} pattern, section {section_index + 1}",
            })
            event_count += 1
        sections.append({
            "name": SECTION_NAMES[(index + section_index) % len(SECTION_NAMES)],
            "gameplay": mode,
            "events": section_events,
        })
        x += rng.randint(35, 70)

    prompt = (
        f"Design a {difficulty} Geometry Dash level with a {style} theme. "
        f"Use {length} connected sections and vary the gameplay modes. "
        "Keep obstacle spacing fair, maintain a readable route, and finish with a memorable finale."
    )
    response = {
        "format": "universe-level-draft-v1",
        "title": f"{style.title()} Circuit {index + 1}",
        "prompt": prompt,
        "difficulty": difficulty,
        "style": style,
        "sections": sections,
        "notes": [
            "Synthetic bootstrap example; validate and playtest before publishing.",
            f"Contains {event_count} ordered object placements.",
        ],
    }
    return {"prompt": prompt, "response": json.dumps(response, ensure_ascii=False, separators=(",", ":"))}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--count", type=int, default=256)
    parser.add_argument("--seed", type=int, default=20261107)
    parser.add_argument("--output", type=Path, default=Path("dataset/train.jsonl"))
    args = parser.parse_args()
    if not 16 <= args.count <= 10000:
        parser.error("--count must be between 16 and 10000.")
    rng = random.Random(args.seed)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("w", encoding="utf-8") as stream:
        for index in range(args.count):
            stream.write(json.dumps(make_example(index, rng), ensure_ascii=False) + "\n")
    print(f"Wrote {args.count} synthetic bootstrap examples to {args.output}")
    print("These are schema/layout-pattern examples, not human-playtested training data.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
