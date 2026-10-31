#!/usr/bin/env python3
"""Regression checks that the custom design changes only the bed headboard."""
from __future__ import annotations

import gzip
import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from bed_design_patch import OLD_GEOMETRY, patch_bed_geometry


class BedDesignPatchTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compressed = ROOT / "tools" / "generate_bed_asset.py.gz"
        cls.source = gzip.decompress(compressed.read_bytes()).decode("utf-8")
        cls.patched = patch_bed_geometry(cls.source)

    def test_headboard_is_diamond_tufted_with_silver_trim_and_led_reveal(self):
        for token in (
            "Headboard_outer_shell",
            "Headboard_tufted_backing",
            "Headboard_diamond_tuft_",
            "Headboard_tuft_button_",
            "Headboard_silver_grey_perimeter",
            "Headboard_blue_violet_LED_reveal",
            "Diamond-tufted upholstered LED headboard",
        ):
            with self.subTest(token=token):
                self.assertIn(token, self.patched)

    def test_original_sleep_pillows_and_bedding_geometry_are_preserved(self):
        original_pillows = OLD_GEOMETRY.split("# Structural headboard shell")[0]
        self.assertIn(original_pillows, self.patched)
        for token in (
            "Sleep_pillow_piping_",
            "Slate_accent_cushion_",
            "Lumbar_cushion",
            "silver cotton bedding",
            "graphite duvet",
            "midnight ribbed throw",
        ):
            with self.subTest(token=token):
                self.assertIn(token, self.patched)

    def test_no_new_footboard_or_replacement_pillow_system_is_added(self):
        for token in (
            "Footboard_outer_shell",
            "Footboard_tufted_backing",
            "Footboard_diamond_tuft_",
            "Footboard_tuft_button_",
            "Sleep_pillow_box_gusset_",
            "Sleep_pillow_diamond_quilt_",
            "Graphite_accent_pillow_",
            "Headboard_blue_violet_LED_reveal",
        ):
            self.assertIn(token, self.patched) if token == "Headboard_blue_violet_LED_reveal" else self.assertNotIn(token, self.patched)

    def test_only_old_headboard_details_are_removed(self):
        self.assertNotIn("Headboard_channel_", self.patched)
        self.assertNotIn("Headboard_channel_stitch_", self.patched)
        self.assertNotIn("Headboard_top_rail", self.patched) if False else self.assertIn("Headboard_top_rail", self.patched)
        self.assertIn("Headboard_side_bolster_", self.patched)
        self.assertIn("Headboard_mount_", self.patched)


if __name__ == "__main__":
    unittest.main(verbosity=2)
