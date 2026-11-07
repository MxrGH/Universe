"""Strict validation for Uni-1 level-draft responses.

This module deliberately has no ML or web-framework dependencies so it can be
tested in the lightweight cross-platform CI job.
"""
from __future__ import annotations

import math

ALLOWED_OBJECTS = {"block", "spike", "saw", "orb", "pad", "object"}
ALLOWED_GAMEPLAY = {"cube"}
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
            if event["type"] == "object":
                object_id = event.get("id")
                if isinstance(object_id, bool) or not isinstance(object_id, int) or not 1 <= object_id <= 5000:
                    raise ValueError("Generic editor objects need an integer id between 1 and 5000.")
                properties = event.get("properties", "")
                if not isinstance(properties, str) or len(properties) > 2000:
                    raise ValueError("Object properties must be a comma-separated string of at most 2,000 characters.")
                if properties:
                    parts = properties.split(",")
                    if len(parts) % 2 or len(parts) > 128:
                        raise ValueError("Object properties must contain up to 64 key/value pairs.")
                    for key, value in zip(parts[::2], parts[1::2]):
                        if not key.isdigit() or key in {"1", "2", "3"}:
                            raise ValueError("Object property keys must be numeric and cannot override object id or coordinates.")
                        if not value or any(ch in value for ch in ";,\\n\\r\\\""):
                            raise ValueError("Object property values must be non-empty and cannot contain save-string delimiters.")
            elif "id" in event or "properties" in event:
                raise ValueError("Use type='object' when specifying a raw Geometry Dash object id or properties.")
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
        raise ValueError("Draft notes must contain text items of at most 500 characters.")

    data["format"] = "universe-level-draft-v1"
    return data, total

MIN_GENERATED_SECTIONS = 6
MAX_GENERATED_SECTIONS = 8
MIN_GENERATED_OBJECTS = 96
MAX_GENERATED_OBJECTS = 144


def validate_generated_level(data: object) -> tuple[dict, int]:
    """Validate the stricter shape expected from a complete Uni-1 level request."""
    draft, object_count = validate_draft_payload(data)
    section_count = len(draft["sections"])
    if not MIN_GENERATED_SECTIONS <= section_count <= MAX_GENERATED_SECTIONS:
        raise ValueError("Generated levels must contain 6 to 8 sections.")
    if not MIN_GENERATED_OBJECTS <= object_count <= MAX_GENERATED_OBJECTS:
        raise ValueError("Generated levels must contain 96 to 144 object placements.")
    return draft, object_count
