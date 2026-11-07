"""Tests for Uni-1's strict model-output validation boundary."""
import unittest

from server import parse_model_draft


def valid_draft():
    sections = []
    x = 0
    for index in range(6):
        events = []
        for _ in range(16):
            x += 30
            events.append({"type": "block", "x": x, "y": 0})
        sections.append({"name": f"Section {index + 1}", "gameplay": "cube", "events": events})
    return {"title": "Validator fixture", "difficulty": "normal", "style": "test", "sections": sections}


class InferenceOutputTests(unittest.TestCase):
    def test_accepts_complete_generated_level(self):
        import json
        draft, count = parse_model_draft(json.dumps(valid_draft()))
        self.assertEqual(count, 96)
        self.assertEqual(len(draft["sections"]), 6)

    def test_rejects_non_json_response_with_clear_error(self):
        with self.assertRaisesRegex(ValueError, "invalid JSON"):
            parse_model_draft("Here is your level!")

    def test_rejects_schema_invalid_json(self):
        import json
        with self.assertRaisesRegex(ValueError, "invalid level draft"):
            parse_model_draft(json.dumps({"sections": []}))


if __name__ == "__main__":
    unittest.main()
