"""Tests for Uni-1 training corpus quality gates."""
import json
import tempfile
import unittest
from pathlib import Path

from generate_dataset import make_example
from validate_dataset import validate_dataset


class DatasetQualityTests(unittest.TestCase):
    def make_corpus(self, path, count=64):
        import random
        rng = random.Random(20261107)
        with path.open("w", encoding="utf-8") as stream:
            for index in range(count):
                row = make_example(index, rng)
                row["synthetic"] = True
                stream.write(json.dumps(row, ensure_ascii=False) + "\n")

    def test_synthetic_bootstrap_requires_explicit_opt_in(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "train.jsonl"
            self.make_corpus(path)
            with self.assertRaisesRegex(ValueError, "synthetic examples are blocked"):
                validate_dataset(path)
            report = validate_dataset(path, allow_synthetic=True)
            self.assertEqual(report["examples"], 64)
            self.assertEqual(report["synthetic_examples"], 64)

    def test_dataset_minimum_is_enforced(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "train.jsonl"
            self.make_corpus(path, 16)
            with self.assertRaisesRegex(ValueError, "at least 32"):
                validate_dataset(path, allow_synthetic=True)

    def test_production_rows_require_review_marker(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "train.jsonl"
            import random
            rng = random.Random(7)
            with path.open("w", encoding="utf-8") as stream:
                for index in range(64):
                    row = make_example(index, rng)
                    response = json.loads(row["response"])
                    response["notes"] = ["Human reviewed and playtested."]
                    row["response"] = json.dumps(response)
                    stream.write(json.dumps(row) + "\n")
            with self.assertRaisesRegex(ValueError, "reviewed=true"):
                validate_dataset(path)

    def test_bootstrap_corpus_covers_multiple_modes_and_verified_editor_objects(self):
        import random
        from object_catalog import OBJECT_IDS
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "train.jsonl"
            rng = random.Random(20261107)
            modes = set()
            generic_ids = set()
            with path.open("w", encoding="utf-8") as stream:
                for index in range(64):
                    row = make_example(index, rng)
                    draft = json.loads(row["response"])
                    modes.update(section["gameplay"] for section in draft["sections"])
                    for section in draft["sections"]:
                        for event in section["events"]:
                            if event["type"] == "object":
                                generic_ids.add(event["id"])
                    row["synthetic"] = True
                    stream.write(json.dumps(row, ensure_ascii=False) + "\n")
            self.assertGreaterEqual(len(modes), 4)
            self.assertGreaterEqual(len(generic_ids), 4)
            self.assertTrue(generic_ids.issubset(set(OBJECT_IDS.values())))
            report = validate_dataset(path, allow_synthetic=True)
            self.assertEqual(report["synthetic_examples"], 64)



if __name__ == "__main__":
    unittest.main()
