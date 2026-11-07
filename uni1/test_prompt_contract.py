"""Ensure Uni-1 is trained, evaluated and served with the same instructions."""
import unittest

from prompts import SYSTEM_PROMPT
import server


class PromptContractTests(unittest.TestCase):
    def test_training_and_serving_share_the_exact_system_prompt(self):
        self.assertEqual(server.SYSTEM_PROMPT, SYSTEM_PROMPT)

    def test_prompt_does_not_restrict_model_to_primitive_cube_layouts(self):
        self.assertIn("generic editor objects", SYSTEM_PROMPT)
        self.assertIn("decorations", SYSTEM_PROMPT)
        self.assertIn("portals", SYSTEM_PROMPT)
        self.assertIn("triggers", SYSTEM_PROMPT)
        self.assertNotIn("Use only block, spike, saw, orb, and pad objects.", SYSTEM_PROMPT)

    def test_prompt_forbids_unverified_ids_and_playtest_claims(self):
        self.assertIn("Never invent an object ID", SYSTEM_PROMPT)
        self.assertIn("Never claim a draft was playtested", SYSTEM_PROMPT)


if __name__ == "__main__":
    unittest.main()
