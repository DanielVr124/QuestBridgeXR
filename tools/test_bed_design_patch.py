#!/usr/bin/env python3
"""Regression checks that only the headboard changes to the single-panel recessed-glow design."""
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

    def test_headboard_is_one_smooth_panel_with_recessed_hidden_angle_glow(self):
        for token in (
            "Headboard_single_upholstered_panel",
            "Headboard_outer_shell",
            "Headboard_rear_backing",
            "Headboard_outer_lip_side_",
            "Headboard_inner_lip_side_",
            "Headboard_outer_lip_edge_",
            "Headboard_inner_lip_edge_",
            "Headboard_panel_perimeter_seam",
            "single smooth charcoal upholstered headboard panel",
            "original bed frame, mattress and bedding retained",
        ):
            with self.subTest(token=token):
                self.assertIn(token, self.patched)
        self.assertIn("taper=.08", self.patched)
        self.assertIn("No exposed LED tape, bulbs, diamond tufting or decorative buttons", self.patched)
        self.assertIn("m.vertex_normals  # cache smooth per-vertex normals for GLB export", self.patched)
        self.assertIn("merged.vertex_normals  # include normals in optimized runtime batches", self.patched)

    def test_diamonds_buttons_channels_and_visible_strip_geometry_are_absent(self):
        for token in (
            "Headboard_diamond_tuft_",
            "Headboard_diamond_stitch_",
            "Headboard_tuft_button_",
            "Headboard_crystal_tuft_button_",
            "headboard_crystal",
            "Headboard_channel_",
            "Headboard_channel_stitch_",
            "Headboard_LED_strip",
        ):
            with self.subTest(token=token):
                self.assertNotIn(token, self.patched)

    def test_original_sleep_pillows_and_bedding_geometry_are_preserved(self):
        original_pillows = OLD_GEOMETRY.split("# Structural headboard shell")[0]
        self.assertIn(original_pillows, self.patched)
        for token in ("Sleep_pillow_piping_", "Slate_accent_cushion_", "Lumbar_cushion", "Sleep_pillow_"):
            with self.subTest(token=token):
                self.assertIn(token, self.patched)

    def test_no_new_footboard_or_replacement_pillow_system_is_added(self):
        for token in (
            "Footboard_outer_shell", "Footboard_tufted_backing",
            "Footboard_diamond_tuft_", "Footboard_tuft_button_",
            "Sleep_pillow_box_gusset_", "Sleep_pillow_diamond_quilt_",
            "Graphite_accent_pillow_",
        ):
            with self.subTest(token=token):
                self.assertNotIn(token, self.patched)

    def test_original_bed_frame_and_headboard_mounts_are_retained(self):
        self.assertIn("Headboard_mount_", self.patched)
        self.assertIn("original bed frame, mattress and bedding retained", self.patched)


if __name__ == "__main__":
    unittest.main(verbosity=2)
