"""Tests for verified editor objects, groups, trigger fields and save properties."""

import pytest

from draft_validation import validate_draft_payload
from object_catalog import OBJECT_IDS, relevant_object_catalog


def draft(event, gameplay="cube"):
    return {
        "title": "Editor object test",
        "prompt": "test",
        "difficulty": "normal",
        "style": "test",
        "sections": [{"name": "Section 1", "gameplay": gameplay, "events": [event]}],
        "notes": [],
    }


def test_catalog_contains_decorations_portals_and_triggers():
    assert OBJECT_IDS["effect_color_trigger"] == 899
    assert OBJECT_IDS["effect_move_trigger"] == 901
    assert any(name.startswith("decor_") for name in OBJECT_IDS)
    assert any(name.startswith("portal_") for name in OBJECT_IDS)


def test_catalog_retrieval_surfaces_trigger_ids():
    catalog = relevant_object_catalog("make a level with color triggers and glow decoration")
    assert "effect_color_trigger = 899" in catalog
    assert "effect_move_trigger = 901" in catalog


def test_generic_trigger_object_with_raw_properties_is_valid():
    event = {
        "action": "add", "x": 300, "y": 0, "type": "object",
        "id": OBJECT_IDS["effect_color_trigger"], "properties": "57,1.2,21,3",
    }
    _, count = validate_draft_payload(draft(event))
    assert count == 1


def test_group_ids_and_structured_trigger_fields_are_serialized():
    event = {
        "x": 300, "y": 0, "type": "object",
        "id": OBJECT_IDS["effect_move_trigger"],
        "groups": [12, 13],
        "target_group": 12,
        "duration": 0.75,
        "move_x": 90,
        "move_y": -30,
        "easing": 2,
        "spawn_triggered": True,
        "spawn_delay": 0.25,
    }
    normalized, _ = validate_draft_payload(draft(event))
    props = normalized["sections"][0]["events"][0]["properties"]
    assert "57,12.13" in props
    assert "51,12" in props
    assert "10,0.75" in props
    assert "28,90" in props
    assert "29,-30" in props
    assert "30,2" in props
    assert "62,1" in props
    assert "63,0.25" in props


def test_group_ids_can_be_attached_to_named_primitives():
    event = {"x": 0, "y": 0, "type": "block", "groups": [4, 5], "scale": 1.5}
    normalized, _ = validate_draft_payload(draft(event))
    result = normalized["sections"][0]["events"][0]
    assert result["type"] == "object"
    assert result["id"] == 1
    assert "57,4.5" in result["properties"]
    assert "32,1.5" in result["properties"]


def test_multiple_game_modes_are_supported_as_section_metadata():
    normalized, _ = validate_draft_payload(draft({"x": 0, "y": 0, "type": "block"}, gameplay="ship"))
    assert normalized["sections"][0]["gameplay"] == "ship"


def test_unknown_object_id_is_rejected():
    with pytest.raises(ValueError, match="verified editor catalogue"):
        validate_draft_payload(draft({"x": 0, "y": 0, "type": "object", "id": 4999}))


@pytest.mark.parametrize("properties", [
    "1,999", "2,10", "3,10", "21,1;1,2", "21,", "21,2,21,3",
    "9999,1", "31,SGVsbG8=",
])
def test_unsafe_or_malformed_properties_are_rejected(properties):
    with pytest.raises(ValueError):
        validate_draft_payload(draft({
            "x": 0, "y": 0, "type": "object",
            "id": OBJECT_IDS["effect_color_trigger"], "properties": properties,
        }))


@pytest.mark.parametrize("groups", [[0], [10000], [1, 1], list(range(1, 12)), "1.2"])
def test_invalid_group_lists_are_rejected(groups):
    with pytest.raises(ValueError):
        validate_draft_payload(draft({
            "x": 0, "y": 0, "type": "object",
            "id": OBJECT_IDS["effect_color_trigger"], "groups": groups,
        }))


@pytest.mark.parametrize("field,value", [
    ("duration", -1), ("duration", 4000), ("opacity", 1.5),
    ("red", 256), ("target_group", 10000), ("touch_triggered", 1),
    ("rotation", float("inf")), ("scale", 0),
])
def test_invalid_structured_properties_are_rejected(field, value):
    with pytest.raises(ValueError):
        validate_draft_payload(draft({
            "x": 0, "y": 0, "type": "object",
            "id": OBJECT_IDS["effect_color_trigger"], field: value,
        }))
