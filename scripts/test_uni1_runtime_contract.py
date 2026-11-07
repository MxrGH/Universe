"""Static deployment/runtime contract checks for Uni-1."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class Uni1RuntimeContractTests(unittest.TestCase):
    def test_completion_reports_validator_object_count(self):
        source = (ROOT / "uni1" / "server.py").read_text(encoding="utf-8")
        self.assertIn("draft, generated_count = validate_draft_payload(draft)", source)
        self.assertIn('"uni1_generated_objects": generated_count', source)
        self.assertNotIn("generated_count = total", source)

    def test_inference_image_contains_the_validator_module(self):
        dockerfile = (ROOT / "uni1" / "Dockerfile").read_text(encoding="utf-8")
        self.assertIn("COPY server.py /app/server.py", dockerfile)
        self.assertIn("COPY draft_validation.py /app/draft_validation.py", dockerfile)

    def test_health_endpoint_requires_loaded_model(self):
        source = (ROOT / "uni1" / "server.py").read_text(encoding="utf-8")
        health = source.split('@app.get("/health")', 1)[1].split('@app.get("/v1/usage")', 1)[0]
        self.assertIn("raise HTTPException(status_code=503", health)
        self.assertIn('"status": "ready"', health)

    def test_compose_is_loopback_bound_and_persists_quota_database(self):
        compose = (ROOT / "uni1" / "compose.yaml").read_text(encoding="utf-8")
        self.assertIn('"127.0.0.1:8000:8000"', compose)
        self.assertIn("uni1-data:/data", compose)
        self.assertIn("./artifacts/uni1-adapter:/models/uni1-adapter:ro", compose)


if __name__ == "__main__":
    unittest.main()
