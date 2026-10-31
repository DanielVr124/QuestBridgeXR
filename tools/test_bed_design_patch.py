#!/usr/bin/env python3
"""Regression checks for the revised headboard and propped sleep pillows."""
from __future__ import annotations
import gzip
import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from bed_design_patch import patch_bed_geometry


class BedDesignPatchTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compressed = ROOT / "tools" / "generate_bed_asset.py.gz"
        cls.source = gzip.decompress(compressed.read_bytes()).decode("utf-8")
        cls.patched = patch_bed_geometry(cls.source)

    def test_new_headboard_and_sleep_pillows_are_present(self):
        for token in (
            "Headboard_full_width_padded_face",
            "Headboard_wrap_wing_",
            "Sleep_pillow_front_stitch_",
            "PILLOW_TILT = math.radians(22.0)",
            "Wide sculpted charcoal wingback headboard",
        ):
            with self.subTest(token=token):
                self.assertIn(token, self.patched)

    def test_old_ribbed_headboard_and_decor_cushions_are_removed(self):
        for token in (
            "Headboard_channel_",
            "Slate_accent_cushion_",
            "Lumbar_cushion",
        ):
            with self.subTest(token=token):
                self.assertNotIn(token, self.patched)


if __name__ == "__main__":
    unittest.main()
