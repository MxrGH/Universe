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




def test_documented_2_2_trigger_properties_are_typed_and_serialized():
    event = {
        "x": 0, "y": 0, "type": "object", "id": OBJECT_IDS["effect_move_trigger"],
        "target_group": 8, "secondary_group": 9, "x_mod": 0.5, "y_mod": -0.25,
        "animation_id": 2, "count": 3, "subtract_count": True, "pickup_mode": 1,
        "item_id": 77, "hold_mode": True, "toggle_mode": 2, "easing_rate": 1.5,
        "comparison": 1, "dual_mode": True, "follow_speed": 2,
        "follow_delay": 0.1, "y_offset": -20, "trigger_on_exit": True,
        "dynamic_block": True, "block_b_id": 44, "custom_rotation_speed": 90,
        "disable_rotation": True, "multi_activate_orbs": True,
        "enable_use_target": True, "target_pos_coordinates": 1,
        "editor_disable": True, "multi_activate_triggers": True,
        "max_speed": 20, "randomize_start": True, "animation_speed": 1.25,
        "linked_group_id": 11, "no_effects": True, "reverse": True,
        "time_mod": 0.75, "no_touch": True,
    }
    normalized, _ = validate_draft_payload(draft(event))
    props = normalized["sections"][0]["events"][0]["properties"]
    for pair in (
        "71,9", "72,0.5", "73,-0.25", "76,2", "77,3", "78,1",
        "79,1", "80,77", "81,1", "82,2", "85,1.5", "88,1",
        "89,1", "90,2", "91,0.1", "92,-20", "93,1", "94,1",
        "95,44", "97,90", "98,1", "99,1", "100,1", "101,1",
        "102,1", "104,1", "105,20", "106,1", "107,1.25",
        "108,11", "116,1", "117,1", "120,0.75", "121,1",
    ):
        assert pair in props


def test_duplicate_aliases_for_same_save_key_are_rejected():
    with pytest.raises(ValueError, match="specified more than once"):
        validate_draft_payload(draft({
            "x": 0, "y": 0, "type": "object",
            "id": OBJECT_IDS["effect_move_trigger"],
            "parent_group": 10, "secondary_group": 11,
        }))

def test_keyframe_point_and_animation_trigger_are_serialized_and_linked():
    events = [
        {"x": 0, "y": 0, "type": "object", "id": 3032,
         "group_id": 22, "keyframe_order": 0, "duration": 0.5,
         "easing": 2, "rotation": 0, "scale": 1},
        {"x": 60, "y": 30, "type": "object", "id": 3032,
         "group_id": 22, "keyframe_order": 1, "duration": 0.75,
         "easing": 3, "rotation": 90, "scale": 1.5},
        {"x": 120, "y": 0, "type": "object", "id": 3033,
         "group_id": 22, "target_group": 40, "parent_group": 41,
         "properties": "72,1,73,1"},
    ]
    normalized, count = validate_draft_payload(draft({"x": 0, "y": 0, "type": "block"}))
    del normalized, count
    normalized, count = validate_draft_payload({
        "title": "Keyframe test", "difficulty": "normal", "sections": [
            {"name": "Animation", "gameplay": "cube", "events": events}
        ]
    })
    assert count == 3
    point = normalized["sections"][0]["events"][0]
    trigger = normalized["sections"][0]["events"][2]
    assert "33,22" in point["properties"]
    assert "115,0" in point["properties"]
    assert "10,0.5" in point["properties"]
    assert "33,22" in trigger["properties"]
    assert "51,40" in trigger["properties"]
    assert "71,41" in trigger["properties"]


def test_keyframe_group_requires_multiple_points_and_trigger():
    events = [
        {"x": 0, "y": 0, "type": "object", "id": 3032,
         "group_id": 7, "keyframe_order": 0},
        {"x": 60, "y": 0, "type": "object", "id": 3033,
         "group_id": 7, "target_group": 8},
    ]
    with pytest.raises(ValueError, match="at least two"):
        validate_draft_payload({
            "title": "Bad keyframe", "difficulty": "normal", "sections": [
                {"name": "Animation", "gameplay": "cube", "events": events}
            ]
        })


def test_camera_static_and_mode_properties_are_serialized():
    events = [
        {"x": 0, "y": 0, "type": "object", "id": 1914,
         "target_group": 30, "duration": 0.5, "easing": 2},
        {"x": 60, "y": 0, "type": "object", "id": 2925,
         "camera_free_mode": True, "camera_edit_settings": True,
         "camera_free_mode_easing": 2, "camera_padding": 20},
    ]
    normalized, _ = validate_draft_payload({
        "title": "Camera test", "difficulty": "normal", "sections": [
            {"name": "Camera", "gameplay": "cube", "events": events}
        ]
    })
    static_props = normalized["sections"][0]["events"][0]["properties"]
    mode_props = normalized["sections"][0]["events"][1]["properties"]
    assert "51,30" in static_props
    assert "111,1" in mode_props
    assert "112,1" in mode_props
    assert "113,2" in mode_props
    assert "114,20" in mode_props


def test_camera_edge_can_release_with_zero_group():
    normalized, _ = validate_draft_payload({
        "title": "Camera edge", "difficulty": "normal", "sections": [
            {"name": "Camera", "gameplay": "cube", "events": [
                {"x": 0, "y": 0, "type": "object", "id": 2062, "target_group": 0}
            ]}
        ]
    })
    assert "51,0" in normalized["sections"][0]["events"][0]["properties"]


def test_invalid_camera_fields_are_rejected_on_wrong_trigger():
    with pytest.raises(ValueError, match="only valid"):
        validate_draft_payload(draft({
            "x": 0, "y": 0, "type": "object", "id": 1913,
            "camera_free_mode": True,
        }))

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
