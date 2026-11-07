"""Static deployment/runtime contract checks for Uni-1."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class Uni1RuntimeContractTests(unittest.TestCase):
    def test_completion_reports_validator_object_count(self):
        source = (ROOT / "uni1" / "server.py").read_text(encoding="utf-8")
        self.assertIn("draft, generated_count = validate_generated_level(draft)", source)
        self.assertIn('"uni1_generated_objects": generated_count', source)
        self.assertNotIn("generated_count = total", source)

    def test_training_uses_tokenized_chat_templates_and_held_out_evaluation(self):
        source = (ROOT / "uni1" / "train.py").read_text(encoding="utf-8")
        self.assertIn("tokenize=True", source)
        self.assertIn("random_split(", source)
        self.assertIn("eval_dataset=eval_dataset", source)
        self.assertIn("load_best_model_at_end=True", source)
        self.assertIn("inspect.signature(TrainingArguments)", source)

    def test_request_input_is_bounded_and_model_id_is_fixed(self):
        source = (ROOT / "uni1" / "server.py").read_text(encoding="utf-8")
        self.assertIn('MAX_INPUT_TOKENS = 8192', source)
        self.assertIn('model: Literal["uni-1"] = "uni-1"', source)
        self.assertIn("prompt_chars > MAX_PROMPT_CHARS", source)
        self.assertIn("prompt_tokens > MAX_INPUT_TOKENS", source)

    def test_inference_image_contains_the_validator_module(self):
        dockerfile = (ROOT / "uni1" / "Dockerfile").read_text(encoding="utf-8")
        self.assertIn("COPY server.py /app/server.py", dockerfile)
        self.assertIn("COPY draft_validation.py /app/draft_validation.py", dockerfile)

    def test_health_endpoint_requires_loaded_model(self):
        source = (ROOT / "uni1" / "server.py").read_text(encoding="utf-8")
        health = source.split('@app.get("/health")', 1)[1].split('@app.get("/v1/usage")', 1)[0]
        self.assertIn("raise HTTPException(status_code=503", health)
        self.assertIn('"status": "ready"', health)

    def test_local_http_is_limited_to_loopback_and_remote_urls_require_https(self):
        source = (ROOT / "src" / "main.cpp").read_text(encoding="utf-8")
        self.assertIn("bool isAllowedUni1ServerURL", source)
        self.assertIn('scheme == "https"', source)
        self.assertIn('host == "localhost"', source)
        self.assertIn('host == "127.0.0.1"', source)
        self.assertIn('host == "[::1]"', source)
        self.assertIn("isAllowedUni1ServerURL(baseUrl)", source)

    def test_compose_is_loopback_bound_and_persists_quota_database(self):
        compose = (ROOT / "uni1" / "compose.yaml").read_text(encoding="utf-8")
        self.assertIn('"127.0.0.1:8000:8000"', compose)
        self.assertIn("uni1-data:/data", compose)
        self.assertIn("./artifacts/uni1-adapter:/models/uni1-adapter:ro", compose)


if __name__ == "__main__":
    unittest.main()
