"""Unit tests for Uni-1 held-out evaluation helpers."""
import json
import tempfile
import unittest
from pathlib import Path

from evaluate import load_eval_prompts, summarize_results


class EvaluationTests(unittest.TestCase):
    def test_evaluation_prompts_require_human_review(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "eval.jsonl"
            path.write_text(json.dumps({"prompt": "Create a cube level"}) + "\n",
                            encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "reviewed=true"):
                load_eval_prompts(path)

    def test_evaluation_prompts_reject_duplicates(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "eval.jsonl"
            row = {"prompt": "Create a cube level", "reviewed": True}
            path.write_text(json.dumps(row) + "\n" + json.dumps(row) + "\n",
                            encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "duplicate"):
                load_eval_prompts(path)

    def test_summary_counts_validity_and_gameplay(self):
        results = [
            {"valid": True, "draft": {
                "sections": [{"gameplay": "cube", "events": [
                    {"type": "block"}, {"type": "spike"}
                ]}, {"gameplay": "ship", "events": [{"type": "object", "id": 15}]}]
            }},
            {"valid": False, "error": "invalid JSON"},
        ]
        report = summarize_results(results)
        self.assertEqual(report["valid_outputs"], 1)
        self.assertEqual(report["invalid_outputs"], 1)
        self.assertEqual(report["valid_rate"], 0.5)
        self.assertEqual(report["average_placements"], 3.0)
        self.assertEqual(report["gameplay_modes"], {"cube": 1, "ship": 1})
        self.assertEqual(report["object_families"].get("decor"), 1)
        self.assertFalse(report["physics_playtested"])


if __name__ == "__main__":
    unittest.main()
