"""Lightweight regression tests for the Uni-1 response validator."""
import math
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "uni1"))
from draft_validation import validate_draft_payload
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "uni1"))
from generate_dataset import make_example
import random


def make_draft(events=None):
    return {
        "title": "Test level",
        "difficulty": "normal",
        "sections": [{
            "name": "Opening",
            "gameplay": "cube",
            "events": events if events is not None else [
                {"x": 0, "y": 10, "type": "block", "action": "add"},
                {"x": 40, "y": 20, "type": "spike"},
            ],
        }],
    }


class Uni1DraftValidationTests(unittest.TestCase):
    def test_valid_draft_is_normalized_and_counted(self):
        draft, count = validate_draft_payload(make_draft())
        self.assertEqual(count, 2)
        self.assertEqual(draft["format"], "universe-level-draft-v1")

    def test_synthetic_bootstrap_examples_pass_production_validator(self):
        rng = random.Random(20261107)
        for index in range(32):
            row = make_example(index, rng)
            draft, count = validate_draft_payload(__import__("json").loads(row["response"]))
            self.assertGreater(count, 0)
            self.assertLessEqual(count, 1000)
            self.assertEqual(draft["format"], "universe-level-draft-v1")

    def test_synthetic_generator_is_reproducible(self):
        first = make_example(7, random.Random(1234))
        second = make_example(7, random.Random(1234))
        self.assertEqual(first, second)

    def test_boolean_coordinate_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "numeric x"):
            validate_draft_payload(make_draft([{"x": True, "y": 0, "type": "block"}]))

    def test_non_finite_coordinate_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "outside the supported range"):
            validate_draft_payload(make_draft([{"x": 0, "y": math.inf, "type": "block"}]))

    def test_out_of_order_coordinates_across_sections_are_rejected(self):
        draft = make_draft()
        draft["sections"].append({
            "name": "Second",
            "gameplay": "ship",
            "events": [{"x": -1, "y": 0, "type": "block"}],
        })
        with self.assertRaisesRegex(ValueError, "non-decreasing"):
            validate_draft_payload(draft)

    def test_non_add_action_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "only add"):
            validate_draft_payload(make_draft([{
                "x": 0, "y": 0, "type": "block", "action": "delete_object"
            }]))

    def test_unsupported_object_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "Unsupported object type"):
            validate_draft_payload(make_draft([{"x": 0, "y": 0, "type": "portal"}]))

    def test_object_limit_is_enforced(self):
        events = [{"x": i, "y": 0, "type": "block"} for i in range(1001)]
        with self.assertRaisesRegex(ValueError, "1,000 object"):
            validate_draft_payload(make_draft(events))

    def test_notes_are_type_and_length_checked(self):
        draft = make_draft()
        draft["notes"] = [123]
        with self.assertRaisesRegex(ValueError, "Draft notes"):
            validate_draft_payload(draft)


if __name__ == "__main__":
    unittest.main()
