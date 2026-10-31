#!/usr/bin/env python3
"""Regression checks that the reference-matched design changes only the headboard."""
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

    def test_headboard_matches_black_diamond_tufted_led_reference(self):
        for token in (
            "Headboard_outer_shell",
            "Headboard_quilt_backing",
            "Headboard_diamond_tuft_",
            "Headboard_diamond_stitch_",
            "Headboard_tuft_button_base_",
            "Headboard_crystal_tuft_button_",
            "Headboard_side_frame_",
            "Headboard_top_rail",
            "Headboard_bottom_rail",
            "Black upholstered diamond-tufted headboard",
            "materials['headboard_crystal']",
        ):
            with self.subTest(token=token):
                self.assertIn(token, self.patched)
        self.assertIn("diamond_rx=.225;diamond_ry=.155", self.patched)
        self.assertIn("radial_steps=20", self.patched)
        self.assertIn("angular_steps=64", self.patched)
        self.assertIn("math.cos(.5*math.pi*radius)**2", self.patched)
        self.assertIn("metallicFactor=0.60", self.patched)
        self.assertIn("roughnessFactor=0.12", self.patched)

    def test_smooth_single_panel_and_old_vertical_channels_are_removed(self):
        for token in (
            "Headboard_single_upholstered_panel",
            "Headboard_inner_lip_side_",
            "Headboard_inner_lip_edge_",
            "Headboard_silver_grey_perimeter",
            "Headboard_channel_",
            "Headboard_channel_stitch_",
            "Single smooth charcoal upholstered headboard panel",
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

    def test_original_bed_frame_and_headboard_mounts_are_retained(self):
        self.assertIn("Headboard_mount_", self.patched)
        self.assertIn("original bed frame, mattress and bedding retained", self.patched)


if __name__ == "__main__":
    unittest.main(verbosity=2)
