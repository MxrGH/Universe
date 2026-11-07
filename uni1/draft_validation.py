"""Strict validation for Uni-1 level-draft responses.

This module deliberately has no ML or web-framework dependencies so it can be
tested in the lightweight cross-platform CI job.
"""
from __future__ import annotations

import math
import re

try:
    from .object_catalog import OBJECT_IDS
except ImportError:  # unittest discover -s uni1
    from object_catalog import OBJECT_IDS

KNOWN_OBJECT_IDS = set(OBJECT_IDS.values())

ALLOWED_OBJECTS = {"block", "spike", "saw", "orb", "pad", "object"}
ALLOWED_GAMEPLAY = {"cube", "ship", "ball", "ufo", "wave", "robot", "spider", "swing", "jetpack"}
DIFFICULTIES = {"easy", "normal", "hard", "insane", "demon"}
MAX_SECTIONS = 100
MAX_OBJECTS = 1000
MAX_COORDINATE = 10_000_000

# Documented Geometry Dash object-save keys. Structured fields below are
# translated into this format before the client serializes the object.
# Source: gd-programming/gd.docs level-object resource (see README citations).
STRUCTURED_PROPERTIES = {
    "flip_x": (4, "bool", 0, 1),
    "flip_y": (5, "bool", 0, 1),
    "rotation": (6, "float", -3600, 3600),
    "red": (7, "int", 0, 255),
    "green": (8, "int", 0, 255),
    "blue": (9, "int", 0, 255),
    "duration": (10, "float", 0, 3600),
    "touch_triggered": (11, "bool", 0, 1),
    "secret_coin_id": (12, "int", 0, 100),
    "special_object_checked": (13, "bool", 0, 1),
    "tint_ground": (14, "bool", 0, 1),
    "player_color_1": (15, "bool", 0, 1),
    "player_color_2": (16, "bool", 0, 1),
    "blending": (17, "bool", 0, 1),
    "editor_layer_1": (20, "int", 0, 100),
    "main_color": (21, "int", 0, 1014),
    "secondary_color": (22, "int", 0, 1014),
    "target_color": (23, "int", 0, 1014),
    "z_layer": (24, "int", -100, 100),
    "z_order": (25, "int", -100000, 100000),
    "move_x": (28, "float", -100000, 100000),
    "move_y": (29, "float", -100000, 100000),
    "easing": (30, "int", 0, 32),
    "scale": (32, "float", 0.05, 20),
    "group_id": (33, "int", 1, 9999),
    "group_parent": (34, "bool", 0, 1),
    "opacity": (35, "float", 0, 1),
    "fade_in": (45, "float", 0, 3600),
    "hold": (46, "float", 0, 3600),
    "fade_out": (47, "float", 0, 3600),
    "pulse_mode": (48, "int", 0, 1),
    "copied_color_id": (50, "int", 0, 1014),
    "target_group": (51, "int", 0, 9999),
    "teleport_y_offset": (54, "float", -100000, 100000),
    "teleport_ease": (55, "bool", 0, 1),
    "pulse_target_type": (52, "int", 0, 2),
    "activate_group": (56, "bool", 0, 1),
    "copy_opacity": (60, "bool", 0, 1),
    "editor_layer_2": (61, "int", 0, 100),
    "lock_to_player_x": (58, "bool", 0, 1),
    "lock_to_player_y": (59, "bool", 0, 1),
    "spawn_triggered": (62, "bool", 0, 1),
    "spawn_delay": (63, "float", 0, 3600),
    "dont_fade": (64, "bool", 0, 1),
    "main_only": (65, "bool", 0, 1),
    "detail_only": (66, "bool", 0, 1),
    "dont_enter": (67, "bool", 0, 1),
    "degrees": (68, "int", -3600, 3600),
    "times_360": (69, "int", 0, 100),
    "strength": (75, "float", 0, 1000),
    "interval": (84, "float", 0, 1000),
    "exclusive": (86, "bool", 0, 1),
    "multi_trigger": (87, "bool", 0, 1),
    "high_detail": (103, "bool", 0, 1),
    "disable_glow": (96, "bool", 0, 1),
    "parent_group": (71, "int", 0, 9999),
    "lock_object_rotation": (70, "bool", 0, 1),
    "keyframe_order": (115, "int", 0, 9999),
    "camera_exit_static": (110, "bool", 0, 1),
    "camera_free_mode": (111, "bool", 0, 1),
    "camera_edit_settings": (112, "bool", 0, 1),
    "camera_free_mode_easing": (113, "int", 0, 32),
    "camera_padding": (114, "float", 0, 1000),
    "scale_x": (128, "float", 0.05, 20),
    "warp_y_angle": (131, "float", -3600, 3600),
    "warp_x_angle": (132, "float", -3600, 3600),
}
GROUP_LIST_KEY = 57
RAW_PROPERTY_KEYS = {4, 5, 6, 7, 8, 9, 10, 11, 13, 14, 15, 16, 17, 19, 20, 21, 22, 23, 24, 25, 28, 29, 30, 32, 33, 34, 35, 41, 42, 45, 46, 47, 48, 50, 51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 75, 76, 77, 78, 79, 80, 81, 82, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 131, 132}


def _format_number(value: int | float) -> str:
    return str(int(value)) if isinstance(value, int) else format(float(value), ".6g")


def normalize_editor_properties(event: dict) -> str:
    """Validate structured trigger/object settings and produce safe save keys."""
    pairs: list[tuple[int, str]] = []
    seen: set[int] = set()

    groups = event.get("groups")
    if groups is not None:
        if not isinstance(groups, list) or not 1 <= len(groups) <= 10:
            raise ValueError("groups must contain between 1 and 10 group IDs.")
        normalized_groups = []
        for group in groups:
            if isinstance(group, bool) or not isinstance(group, int) or not 1 <= group <= 9999:
                raise ValueError("Group IDs must be integers from 1 to 9999.")
            if group in normalized_groups:
                raise ValueError("Group IDs cannot be duplicated on one object.")
            normalized_groups.append(group)
        pairs.append((GROUP_LIST_KEY, ".".join(str(group) for group in normalized_groups)))
        seen.add(GROUP_LIST_KEY)

    for field, (key, kind, minimum, maximum) in STRUCTURED_PROPERTIES.items():
        if field not in event:
            continue
        value = event[field]
        if kind == "bool":
            if not isinstance(value, bool):
                raise ValueError(f"{field} must be a boolean.")
            numeric = int(value)
        elif kind == "int":
            if isinstance(value, bool) or not isinstance(value, int):
                raise ValueError(f"{field} must be an integer.")
            numeric = value
        else:
            if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value):
                raise ValueError(f"{field} must be a finite number.")
            numeric = float(value)
        if not minimum <= numeric <= maximum:
            raise ValueError(f"{field} must be between {minimum} and {maximum}.")
        if key in seen:
            raise ValueError(f"Property key {key} was specified more than once.")
        pairs.append((key, _format_number(numeric)))
        seen.add(key)

    raw = event.get("properties", "")
    if not isinstance(raw, str) or len(raw) > 2000:
        raise ValueError("properties must be a comma-separated string of at most 2,000 characters.")
    if raw:
        parts = raw.split(",")
        if len(parts) % 2 or len(parts) > 128:
            raise ValueError("properties must contain up to 64 key/value pairs.")
        for key_text, value in zip(parts[::2], parts[1::2]):
            if not key_text.isdigit():
                raise ValueError("Raw property keys must be numeric.")
            key = int(key_text)
            if key not in RAW_PROPERTY_KEYS or key in {1, 2, 3}:
                raise ValueError(f"Raw property key {key} is not supported.")
            if key in seen:
                raise ValueError(f"Property key {key} was specified more than once.")
            if not value or not re.fullmatch(r"-?\d+(?:\.\d+)?(?:\.-?\d+(?:\.\d+)?)*", value):
                raise ValueError(f"Raw property {key} must contain a numeric value or dot-separated numeric list.")
            pairs.append((key, value))
            seen.add(key)

    if len(pairs) > 64:
        raise ValueError("An object may contain at most 64 additional properties.")
    return ",".join(part for key, value in pairs for part in (str(key), value))


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
                if isinstance(object_id, bool) or not isinstance(object_id, int) or object_id not in KNOWN_OBJECT_IDS:
                    raise ValueError("Generic editor objects need an exact integer id from the verified editor catalogue.")
                event["properties"] = normalize_editor_properties(event)
            elif "id" in event or "properties" in event or "groups" in event or any(field in event for field in STRUCTURED_PROPERTIES):
                primitive_ids = {"block": 1, "spike": 8, "saw": 39, "orb": 36, "pad": 35}
                if event["type"] not in primitive_ids or "id" in event or "properties" in event:
                    raise ValueError("Only named primitives may be converted to grouped editor objects.")
                event["id"] = primitive_ids[event["type"]]
                event["properties"] = normalize_editor_properties(event)
                event["type"] = "object"
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

    # Keyframe points (3032) and their activation trigger (3033) form a
    # linked animation: all points share one group, each has a unique order,
    # and an animation trigger targets that group's points.
    keyframe_points: dict[int, list[int]] = {}
    keyframe_triggers: set[int] = set()
    camera_ids = {1913, 1914, 1916, 2015, 2016, 2062, 2901, 2925}
    for section in sections:
        for event in section["events"]:
            if event.get("type") != "object":
                continue
            object_id = event.get("id")
            if object_id in camera_ids:
                camera_fields = {
                    "camera_exit_static", "camera_free_mode", "camera_edit_settings",
                    "camera_free_mode_easing", "camera_padding",
                }
                if any(field in event for field in camera_fields) and object_id not in {1914, 2925}:
                    raise ValueError("Camera mode/static settings are only valid on Static Camera or Camera Mode triggers.")
                if object_id == 1914 and "target_group" not in event and not event.get("camera_exit_static", False):
                    raise ValueError("Static Camera requires target_group unless camera_exit_static is enabled.")
                if object_id == 2062 and "target_group" not in event:
                    raise ValueError("Edge Camera requires target_group; use 0 to release the edge.")
                if object_id == 2925 and "camera_free_mode" not in event:
                    raise ValueError("Camera Mode requires camera_free_mode.")
            if object_id == 3032:
                group = event.get("group_id")
                order = event.get("keyframe_order")
                if group is None or order is None:
                    raise ValueError("Keyframe Point requires group_id and keyframe_order.")
                keyframe_points.setdefault(group, []).append(order)
            elif object_id == 3033:
                group = event.get("group_id")
                if group is None or "target_group" not in event:
                    raise ValueError("Keyframe Animation Trigger requires group_id (animation group) and target_group.")
                keyframe_triggers.add(group)

    for group, orders in keyframe_points.items():
        if len(orders) < 2:
            raise ValueError(f"Keyframe animation group {group} needs at least two Keyframe Points.")
        if len(orders) != len(set(orders)):
            raise ValueError(f"Keyframe animation group {group} has duplicate keyframe_order values.")
        if group not in keyframe_triggers:
            raise ValueError(f"Keyframe animation group {group} has no Keyframe Animation Trigger.")
    for group in keyframe_triggers:
        if group not in keyframe_points:
            raise ValueError(f"Keyframe Animation Trigger group {group} has no Keyframe Points.")

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
