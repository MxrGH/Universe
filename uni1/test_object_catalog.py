"""Tests for verified object IDs and raw editor-property serialization schema."""

import pytest

from draft_validation import validate_draft_payload
from object_catalog import OBJECT_IDS, relevant_object_catalog


def draft(event):
    return {
        "title": "Editor object test",
        "prompt": "test",
        "difficulty": "normal",
        "style": "test",
        "sections": [{"name": "Section 1", "gameplay": "cube", "events": [event]}],
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


def test_generic_trigger_object_with_properties_is_valid():
    event = {
        "action": "add",
        "x": 300,
        "y": 0,
        "type": "object",
        "id": OBJECT_IDS["effect_color_trigger"],
        "properties": "57,1.2,21,3",
    }
    _, count = validate_draft_payload(draft(event))
    assert count == 1


def test_unknown_object_id_is_rejected():
    with pytest.raises(ValueError, match="verified editor catalogue"):
        validate_draft_payload(draft({"x": 0, "y": 0, "type": "object", "id": 4999}))


@pytest.mark.parametrize("properties", ["1,999", "2,10", "3,10", "21,1;1,2", "21,"])
def test_unsafe_or_malformed_properties_are_rejected(properties):
    with pytest.raises(ValueError):
        validate_draft_payload(draft({
            "x": 0, "y": 0, "type": "object",
            "id": OBJECT_IDS["effect_color_trigger"],
            "properties": properties,
        }))
