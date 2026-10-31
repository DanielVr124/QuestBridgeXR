#!/usr/bin/env python3
"""Regression checks for the diamond-tufted LED bed and quilted side-sleeper pillows."""
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

    def test_diamond_tufted_headboard_has_silver_edge_and_led(self):
        for token in (
            "Headboard_tufted_backing",
            "Headboard_diamond_tuft_",
            "Headboard_tuft_button_",
            "Headboard_silver_grey_perimeter",
            "Headboard_blue_violet_LED_reveal",
            "Black diamond-tufted LED headboard",
        ):
            with self.subTest(token=token):
                self.assertIn(token, self.patched)

    def test_footboard_matches_headboard_diamond_design(self):
        for token in (
            "Footboard_outer_shell",
            "Footboard_tufted_backing",
            "Footboard_diamond_tuft_",
            "Footboard_tuft_button_",
            "Footboard_silver_grey_perimeter",
            "Footboard_blue_violet_LED_reveal",
        ):
            with self.subTest(token=token):
                self.assertIn(token, self.patched)

    def test_sleep_pillows_have_gussets_quilting_and_soft_corners(self):
        for token in (
            "Sleep_pillow_box_gusset_",
            "gusset_uv,'pillow')",
            "Sleep_pillow_top_welt_",
            "Sleep_pillow_diamond_quilt_",
            "Sleep_pillow_corner_dart_",
            "PILLOW_TILT = math.radians(10.0)",
            "def pillow_outline(u, scale=1.0):",
            "PILLOW_HALF_WIDTH * scale",
            "PILLOW_HALF_DEPTH * scale",
        ):
            with self.subTest(token=token):
                self.assertIn(token, self.patched)

    def test_old_ribbed_headboard_and_old_decor_cushions_are_removed(self):
        for token in (
            "Headboard_channel_",
            "Headboard_channel_stitch_",
            "Slate_accent_cushion_",
            "Lumbar_cushion",
            "Wide sculpted charcoal wingback headboard",
        ):
            with self.subTest(token=token):
                self.assertNotIn(token, self.patched)


if __name__ == "__main__":
    unittest.main()
