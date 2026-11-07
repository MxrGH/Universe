#!/usr/bin/env python3
"""Static contract tests for Universe's in-game controls and Geode settings."""

from __future__ import annotations

import json
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = (ROOT / "src" / "main.cpp").read_text(encoding="utf-8")
SETTINGS = json.loads((ROOT / "mod.json").read_text(encoding="utf-8"))["settings"]


class UIContractTests(unittest.TestCase):
    def test_toggle_controls_reference_declared_settings(self):
        toggles = re.findall(
            r'addToggle\(\s*"[^"]*"\s*,\s*"([^"]+)"\s*,\s*(\d+)\s*,',
            SOURCE,
        )
        self.assertTrue(toggles, "No toggle controls were found.")
        missing = sorted({setting for setting, _ in toggles if setting not in SETTINGS})
        self.assertEqual(missing, [], f"UI toggles reference missing settings: {missing}")

    def test_keybind_listeners_reference_keybind_settings(self):
        listeners = re.findall(
            r'listenForKeybindSettingPresses\(\s*"([^"]+)"', SOURCE
        )
        self.assertTrue(listeners, "No keybind listeners were found.")
        invalid = sorted(
            key for key in set(listeners)
            if key not in SETTINGS or SETTINGS[key].get("type") != "keybind"
        )
        self.assertEqual(invalid, [], f"Listeners reference invalid keybind settings: {invalid}")

    def test_every_visible_action_tag_has_a_handler(self):
        literal_actions = re.findall(
            r'addAction\(\s*[^,]+,\s*(\d+)\s*,\s*\{', SOURCE
        )
        toggle_actions = re.findall(
            r'addToggle\(\s*"[^"]*"\s*,\s*"[^"]+"\s*,\s*(\d+)\s*,',
            SOURCE,
        )
        action_tags = {int(tag) for tag in literal_actions + toggle_actions}
        self.assertTrue(action_tags, "No action tags were found.")

        handler = SOURCE.split("void onAction(CCObject* sender)", 1)[1].split(
            "void closePanel", 1
        )[0]
        handled = {
            int(tag) for tag in re.findall(r'case\s+(\d+)\s*:', handler)
        }
        handled.update(
            int(tag) for tag in re.findall(r'tag\s*==\s*(\d+)', handler)
        )
        ranges = [
            (int(start), int(end))
            for start, end in re.findall(
                r'tag\s*>=\s*(\d+)\s*&&\s*tag\s*<=\s*(\d+)', handler
            )
        ]
        missing = sorted(
            tag for tag in action_tags
            if tag not in handled and not any(start <= tag <= end for start, end in ranges)
        )
        self.assertEqual(missing, [], f"Visible controls have no action handler: {missing}")


if __name__ == "__main__":
    unittest.main()
