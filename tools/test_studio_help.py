#!/usr/bin/env python3
"""Guard detailed tooltip explanations against accidental removal."""
from __future__ import annotations

import unittest

from questbridge_studio import SETTING_HELP


class StudioHelpTests(unittest.TestCase):
    def test_every_studio_setting_has_a_detailed_hover_explanation(self):
        self.assertEqual(len(SETTING_HELP), 75)
        self.assertEqual(len(set(SETTING_HELP)), 75)
        for key, explanation in SETTING_HELP.items():
            with self.subTest(setting=key):
                self.assertIn(".", explanation)
                self.assertGreaterEqual(len(explanation), 80)


if __name__ == "__main__":
    unittest.main(verbosity=2)
