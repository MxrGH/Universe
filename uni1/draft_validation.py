"""Strict validation for Uni-1 level-draft responses.

This module deliberately has no ML or web-framework dependencies so it can be
tested in the lightweight cross-platform CI job.
"""
from __future__ import annotations

import math

ALLOWED_OBJECTS = {"block", "spike", "saw", "orb", "pad", "deco"}
ALLOWED_GAMEPLAY = {"cube", "ship", "ball", "ufo", "wave", "robot", "spider", "swing", "jetpack"}
DIFFICULTIES = {"easy", "normal", "hard", "insane", "demon"}
MAX_SECTIONS = 100
MAX_OBJECTS = 1000
MAX_COORDINATE = 10_000_000


def validate_draft_payload(data: object) -> tuple[dict, int]:
    """Validate and normalize a generated draft; return (draft, object_count)."""
    if not isinstance(data, dict):
        raise ValueError("Draft must be a JSON object.")

    for field, default, limit in (
        ("title", "Untitled Uni-1 Draft", 120),
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

    sections = data.get("sections")
    if not isinstance(sections, list) or not sections or len(sections) > MAX_SECTIONS:
        raise ValueError("Draft must contain between 1 and 100 sections.")

    previous_x = float("-inf")
    total = 0
    for index, section in enumerate(sections):
        if not isinstance(section, dict):
            raise ValueError("Every section must be an object.")
        name = section.get("name", f"Section {index + 1}")
        gameplay = section.get("gameplay", "cube")
        events = section.get("events")
        if not isinstance(name, str) or len(name) > 120:
            raise ValueError("Section names must be text of at most 120 characters.")
        if not isinstance(gameplay, str) or gameplay not in ALLOWED_GAMEPLAY:
            raise ValueError(f"Unsupported gameplay mode: {gameplay!r}")
        if not isinstance(events, list) or not events:
            raise ValueError("Every section must contain at least one event.")
        section["name"] = name
        section["gameplay"] = gameplay

        for event in events:
            if not isinstance(event, dict):
                raise ValueError("Every event must be an object.")
            if not isinstance(event.get("type"), str) or event.get("type") not in ALLOWED_OBJECTS:
                raise ValueError(f"Unsupported object type: {event.get('type')!r}")
            if event.get("action", "add") != "add":
                raise ValueError("Uni-1 generation may only add new objects.")
            for axis in ("x", "y"):
                value = event.get(axis)
                if isinstance(value, bool) or not isinstance(value, (int, float)):
                    raise ValueError(f"Every event needs a numeric {axis} coordinate.")
                if not math.isfinite(value) or abs(value) > MAX_COORDINATE:
                    raise ValueError(f"Event {axis} coordinate is outside the supported range.")
            if event["x"] < previous_x:
                raise ValueError("Event x coordinates must be in non-decreasing order.")
            previous_x = event["x"]
            note = event.get("note", "")
            if not isinstance(note, str) or len(note) > 240:
                raise ValueError("Event notes must be text of at most 240 characters.")
            total += 1
            if total > MAX_OBJECTS:
                raise ValueError("Uni-1 output exceeds the 1,000 object server limit.")

    notes = data.get("notes", [])
    if not isinstance(notes, list) or len(notes) > 100:
        raise ValueError("Draft notes must be a list containing at most 100 items.")
    if any(not isinstance(note, str) or len(note) > 500 for note in notes):
        raise ValueError("Each draft note must be text of at most 500 characters.")

    data["format"] = "universe-level-draft-v1"
    return data, total
