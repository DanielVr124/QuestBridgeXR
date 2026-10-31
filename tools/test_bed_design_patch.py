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

    def test_headboard_is_one_smooth_upholstered_panel_with_attached_led(self):
        for token in (
            "Headboard_outer_shell",
            "Headboard_single_upholstered_panel",
            "Headboard_inner_lip_side_", "Headboard_inner_lip_edge_",
            "Headboard_silver_grey_perimeter",
            "Single smooth charcoal upholstered headboard panel",
        ):
            with self.subTest(token=token):
                self.assertIn(token, self.patched)
        self.assertIn("(0,1.175,-1.255),(2.15,1.38,.22)", self.patched)
        self.assertIn("'charcoal',.38", self.patched)

    def test_no_diamond_tufts_or_buttons_remain_on_the_headboard(self):
        for token in (
            "Headboard_diamond_tuft_",
            "Headboard_tuft_button_",
            "Headboard_tufted_backing",
            "Headboard_channel_",
            "Headboard_channel_stitch_",
            "Headboard_blue_violet_LED_reveal", "materials['led']=PBRMaterial",
        ):
            with self.subTest(token=token):
                self.assertNotIn(token, self.patched)

    def test_original_sleep_pillows_and_bedding_geometry_are_preserved(self):
        original_pillows = OLD_GEOMETRY.split("# Structural headboard shell")[0]
        self.assertIn(original_pillows, self.patched)
        for token in (
            "Sleep_pillow_piping_",
            "Slate_accent_cushion_",
            "Lumbar_cushion",
            "Sleep_pillow_",
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
        ):
            with self.subTest(token=token):
                self.assertNotIn(token, self.patched)

    def test_original_headboard_wings_and_mounting_are_retained(self):
        self.assertIn("Headboard_top_rail", self.patched)
        self.assertIn("Headboard_side_bolster_", self.patched)
        self.assertIn("Headboard_mount_", self.patched)
        self.assertIn("original bed frame, mattress and bedding retained", self.patched)


if __name__ == "__main__":
    unittest.main(verbosity=2)
