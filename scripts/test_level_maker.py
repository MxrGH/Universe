#!/usr/bin/env python3
"""Regression tests for the Universe level blueprint generator."""

import math
import unittest

from level_maker import offline_draft, validate_draft


def draft_with(events):
    return {
        "sections": [{"name": "Test", "events": events}],
    }


class LevelMakerTests(unittest.TestCase):
    def test_offline_draft_is_deterministic(self):
        first = offline_draft("Neon wave", "hard", 32, "wave", 1234)
        second = offline_draft("Neon wave", "hard", 32, "wave", 1234)
        self.assertEqual(first, second)
        self.assertGreater(sum(len(s["events"]) for s in first["sections"]), 0)

    def test_valid_draft_is_normalized(self):
        result = validate_draft(draft_with([
            {"x": 0, "y": 10, "type": "block"},
            {"x": 100, "y": 20.5, "type": "spike"},
        ]))
        self.assertEqual(result["format"], "universe-level-draft-v1")

    def test_boolean_coordinate_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "numeric x"):
            validate_draft(draft_with([{"x": True, "y": 0, "type": "block"}]))

    def test_non_finite_coordinate_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "finite y"):
            validate_draft(draft_with([{"x": 0, "y": math.nan, "type": "block"}]))

    def test_out_of_order_events_are_rejected(self):
        with self.assertRaisesRegex(ValueError, "non-decreasing"):
            validate_draft(draft_with([
                {"x": 20, "y": 0, "type": "block"},
                {"x": 10, "y": 0, "type": "spike"},
            ]))

    def test_unknown_object_type_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "Unsupported object type"):
            validate_draft(draft_with([{"x": 0, "y": 0, "type": "unknown"}]))

    def test_coordinate_range_is_bounded(self):
        with self.assertRaisesRegex(ValueError, "outside the supported range"):
            validate_draft(draft_with([{"x": 10_000_001, "y": 0, "type": "block"}]))


if __name__ == "__main__":
    unittest.main()
