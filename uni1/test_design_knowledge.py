"""Regression checks for the imported Uni-1 design guidance."""

from design_knowledge import DESIGN_GUIDANCE


def test_guidance_covers_gameplay_and_visual_design():
    assert "GAMEPLAY MODE CONTRACTS" in DESIGN_GUIDANCE
    assert "DECORATION WORKFLOW" in DESIGN_GUIDANCE
    assert "STYLE RECIPES" in DESIGN_GUIDANCE
    assert "playtesting" in DESIGN_GUIDANCE.lower()


def test_compatibility_override_keeps_current_schema_restricted():
    override = DESIGN_GUIDANCE.split("CURRENT UNI-1 COMPATIBILITY OVERRIDE:", 1)[1]
    assert "cube gameplay" in override
    assert "block, spike, saw, orb, and pad" in override
    assert "Do not emit decoration placeholders, portals, triggers" in override
