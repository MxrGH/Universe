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
MODES = ["cube"]
OBJECTS = ["block", "spike", "saw", "orb", "pad"]
SECTION_NAMES = [
    "Opening", "Warm-up", "Main Theme", "Variation", "Challenge", "Finale",
    "Transition", "Climax", "Outro",
]


def make_example(index: int, rng: random.Random) -> dict:
    style = STYLES[index % len(STYLES)]
    difficulty = DIFFICULTIES[(index // 3) % len(DIFFICULTIES)]
    section_count = rng.randint(6, 8)
    total_objects = rng.randint(96, 144)
    base_count, remainder = divmod(total_objects, section_count)
    sections = []
    x = 0
    event_count = 0

    for section_index in range(section_count):
        # The editor currently places geometry only; it does not add mode portals.
        mode = "cube"
        event_slots = base_count + (1 if section_index < remainder else 0)
        section_events = []
        spacing = rng.randint(36, 48)
        base_y = rng.choice([-30, 0, 30])
        for slot in range(event_slots):
            x += spacing
            phase = slot % 12
            if phase in (0, 1, 4, 5, 9, 10):
                object_type = "block"
            elif phase in (2, 7):
                object_type = "spike"
            elif phase == 3:
                object_type = "orb"
            elif phase == 8:
                object_type = "pad"
            elif phase == 11 and difficulty in ("hard", "insane"):
                object_type = "saw"
            else:
                object_type = rng.choice(["block", "spike"])
            y = base_y + rng.choice([-30, 0, 0, 30])
            if object_type in ("orb", "saw"):
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
        x += rng.randint(90, 140)

    prompt = (
        f"Design a {difficulty} Geometry Dash cube-mode level with a {style} theme. "
        f"Use {section_count} connected sections and exactly {event_count} ordered placements. "
        "Use readable routes, varied but fair obstacle patterns, and clear transitions. "
        "Do not add portals or change the player mode; all sections remain cube gameplay."
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
            "Cube mode only; no mode portals or triggers are generated.",
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
