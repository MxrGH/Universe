"""Production adapter-manifest gates."""
import tempfile
import unittest
from pathlib import Path

from model_manifest import build_model_manifest, validate_model_manifest


def fixture_manifest():
    return {
        "format": "uni1-lora-manifest-v1",
        "base_model": "Qwen/Qwen2.5-0.5B-Instruct",
        "training_data_sha256": "a" * 64,
        "dataset": {"synthetic_examples": 0, "reviewed_examples": 32},
        "training_examples": 28,
        "validation_examples": 4,
        "validation_metrics": {"eval_loss": 0.5},
        "synthetic_bootstrap": False,
        "adapter_type": "peft-lora",
    }


class ModelManifestTests(unittest.TestCase):
    def test_accepts_reviewed_production_adapter_manifest(self):
        manifest = validate_model_manifest(
            fixture_manifest(), expected_base_model="Qwen/Qwen2.5-0.5B-Instruct"
        )
        self.assertEqual(manifest["dataset"]["reviewed_examples"], 32)

    def test_rejects_synthetic_bootstrap_adapter(self):
        manifest = fixture_manifest()
        manifest["synthetic_bootstrap"] = True
        with self.assertRaisesRegex(ValueError, "synthetic-bootstrap"):
            validate_model_manifest(manifest, expected_base_model=manifest["base_model"])

    def test_rejects_wrong_base_model(self):
        with self.assertRaisesRegex(ValueError, "does not match"):
            validate_model_manifest(fixture_manifest(), expected_base_model="other/model")

    def test_rejects_insufficient_reviewed_examples(self):
        manifest = fixture_manifest()
        manifest["dataset"]["reviewed_examples"] = 31
        with self.assertRaisesRegex(ValueError, "at least 32"):
            validate_model_manifest(manifest, expected_base_model=manifest["base_model"])

    def test_builds_reproducible_dataset_fingerprint(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "train.jsonl"
            path.write_text('{"prompt":"reviewed"}\\n', encoding="utf-8")
            manifest = build_model_manifest(
                base_model="base",
                dataset_path=path,
                dataset_report={"synthetic_examples": 0, "reviewed_examples": 32},
                training_examples=28,
                validation_examples=4,
                validation_metrics={"eval_loss": 0.5},
                seed=7,
                epochs=3,
                learning_rate=0.0002,
                max_length=8192,
                synthetic_bootstrap=False,
            )
            self.assertEqual(len(manifest["training_data_sha256"]), 64)
            self.assertFalse(manifest["synthetic_bootstrap"])


if __name__ == "__main__":
    unittest.main()
