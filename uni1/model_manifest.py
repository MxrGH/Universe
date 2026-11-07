"""Integrity and release gates for trained Uni-1 adapters."""
from __future__ import annotations

import hashlib
import re
from pathlib import Path


def build_model_manifest(
    *,
    base_model: str,
    dataset_path: Path,
    dataset_report: dict,
    training_examples: int,
    validation_examples: int,
    validation_metrics: dict,
    seed: int,
    epochs: float,
    learning_rate: float,
    max_length: int,
    synthetic_bootstrap: bool,
) -> dict:
    digest = hashlib.sha256(dataset_path.read_bytes()).hexdigest()
    return {
        "format": "uni1-lora-manifest-v1",
        "base_model": base_model,
        "training_data_sha256": digest,
        "dataset": dataset_report,
        "training_examples": training_examples,
        "validation_examples": validation_examples,
        "validation_metrics": validation_metrics,
        "seed": seed,
        "epochs": epochs,
        "learning_rate": learning_rate,
        "max_length": max_length,
        "synthetic_bootstrap": bool(synthetic_bootstrap),
        "adapter_type": "peft-lora",
    }


def validate_model_manifest(manifest: object, *, expected_base_model: str) -> dict:
    if not isinstance(manifest, dict):
        raise ValueError("manifest must be a JSON object.")
    if manifest.get("format") != "uni1-lora-manifest-v1":
        raise ValueError("unsupported or missing manifest format.")
    if manifest.get("adapter_type") != "peft-lora":
        raise ValueError("adapter type is not PEFT LoRA.")
    if manifest.get("base_model") != expected_base_model:
        raise ValueError(
            f"adapter base model {manifest.get('base_model')!r} does not match configured "
            f"base model {expected_base_model!r}."
        )
    digest = manifest.get("training_data_sha256")
    if not isinstance(digest, str) or re.fullmatch(r"[0-9a-f]{64}", digest) is None:
        raise ValueError("training data SHA-256 fingerprint is missing or invalid.")
    if manifest.get("synthetic_bootstrap") is not False:
        raise ValueError("synthetic-bootstrap adapters are experimental and cannot be served.")
    dataset = manifest.get("dataset")
    if not isinstance(dataset, dict):
        raise ValueError("dataset quality report is missing.")
    if dataset.get("synthetic_examples") != 0:
        raise ValueError("production adapters must contain zero synthetic training examples.")
    reviewed = dataset.get("reviewed_examples")
    if isinstance(reviewed, bool) or not isinstance(reviewed, int) or reviewed < 32:
        raise ValueError("production adapters require at least 32 human-reviewed examples.")
    training_examples = manifest.get("training_examples")
    validation_examples = manifest.get("validation_examples")
    if (isinstance(training_examples, bool) or not isinstance(training_examples, int)
            or training_examples < 1):
        raise ValueError("manifest training example count is invalid.")
    if (isinstance(validation_examples, bool) or not isinstance(validation_examples, int)
            or validation_examples < 1):
        raise ValueError("manifest validation example count is invalid.")
    metrics = manifest.get("validation_metrics")
    if not isinstance(metrics, dict) or not metrics:
        raise ValueError("held-out training metrics are missing.")
    return manifest
