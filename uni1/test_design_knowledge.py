"""Regression checks for Uni-1's expanded editor-generation contract."""

from design_knowledge import DESIGN_GUIDANCE


def test_guidance_covers_gameplay_and_visual_design():
    assert "GAMEPLAY MODE CONTRACTS" in DESIGN_GUIDANCE
    assert "DECORATION WORKFLOW" in DESIGN_GUIDANCE
    assert "STYLE RECIPES" in DESIGN_GUIDANCE
    assert "playtesting" in DESIGN_GUIDANCE.lower()


def test_guidance_explains_generic_objects_and_trigger_limits():
    override = DESIGN_GUIDANCE.split("CURRENT UNI-1 COMPATIBILITY OVERRIDE:", 1)[1]
    assert 'type="object"' in override
    assert "numeric id from the verified object catalogue" in override
    assert "Do not invent trigger property keys or values" in override
    assert "Never claim a level was playtested" in override
