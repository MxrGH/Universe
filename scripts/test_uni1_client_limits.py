"""Keep the native editor and Uni-1 server object ceilings in sync."""
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class Uni1ClientLimitTests(unittest.TestCase):
    def test_native_editor_limit_matches_server_validator(self):
        cpp = (ROOT / "src" / "main.cpp").read_text(encoding="utf-8")
        validator = (ROOT / "uni1" / "draft_validation.py").read_text(encoding="utf-8")

        client_match = re.search(
            r"kMaxGeneratedObjects\s*=\s*([0-9']+)", cpp
        )
        server_match = re.search(
            r"^MAX_OBJECTS\s*=\s*([0-9_]+)$", validator, re.MULTILINE
        )
        self.assertIsNotNone(client_match, "Native editor placement limit is missing.")
        self.assertIsNotNone(server_match, "Uni-1 server placement limit is missing.")

        client_limit = int(client_match.group(1).replace("'", ""))
        server_limit = int(server_match.group(1).replace("_", ""))
        self.assertEqual(client_limit, server_limit)
        self.assertEqual(client_limit, 1000)

    def test_client_quota_day_uses_utc_like_the_server(self):
        cpp = (ROOT / "src" / "main.cpp").read_text(encoding="utf-8")
        self.assertIn("std::gmtime(&now)", cpp)
        self.assertNotIn("std::localtime(&now)", cpp)

    def test_rejection_message_reports_the_actual_limit(self):
        cpp = (ROOT / "src" / "main.cpp").read_text(encoding="utf-8")
        self.assertIn("1,000 generated objects", cpp)
        self.assertNotIn("1,000,000 generated objects", cpp)


if __name__ == "__main__":
    unittest.main()
