#!/usr/bin/env python3
"""Regression checks for the Brambley-reference headboard-only redesign."""
from __future__ import annotations
import gzip
import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from bed_design_patch import OLD_GEOMETRY, NEW_GEOMETRY, patch_bed_geometry


class BedDesignPatchTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compressed = ROOT / "tools" / "generate_bed_asset.py.gz"
        cls.source = gzip.decompress(compressed.read_bytes()).decode("utf-8")
        cls.patched = patch_bed_geometry(cls.source)

    def test_brambley_headboard_components_are_present(self):
        for token in (
            "Headboard_structural_backing",
            "Headboard_alligator_side_band_",
            "Headboard_alligator_scale_",
            "Headboard_mirror_bevel_",
            "Headboard_inner_shadow_reveal",
            "Headboard_recessed_LED_channel_",
            "Headboard_crystal_tufted_silver_insert",
            "Headboard_crystal_button_setting_",
            "Headboard_crystal_tuft_button_",
            "Headboard_tuft_seam_",
            "Headboard_mount_",
        ):
            with self.subTest(token=token):
                self.assertIn(token, self.patched)

        self.assertIn("(1.055,.685,.128),'sheet'", self.patched)
        self.assertIn("tuft_x = (-.82,-.41,0,.41,.82)", self.patched)
        self.assertIn("tuft_y = (1.705,1.355,1.015,.665)", self.patched)
        self.assertIn("recessed perimeter RGB backlight", self.patched)
        self.assertEqual(self.patched.count("Headboard_crystal_tufted_silver_insert"), 1)

    def test_headboard_is_not_the_failed_plain_slab_or_vertical_ribs(self):
        for obsolete in (
            "Headboard_single_soft_panel",
            "Headboard_channel_",
            "Headboard_channel_stitch_",
            "Headboard_side_bolster_",
            "Headboard_top_rail",
        ):
            with self.subTest(obsolete=obsolete):
                self.assertNotIn(obsolete, self.patched)

    def test_original_bed_body_bedding_and_existing_pillow_layout_are_retained(self):
        restored = self.patched.replace(NEW_GEOMETRY, OLD_GEOMETRY, 1)
        restored = restored.replace(
            "        m.vertex_normals  # cache smooth per-vertex normals for GLB export\n", "", 1
        )
        restored = restored.replace(
            "    merged.vertex_normals  # include normals in optimized runtime batches\n", "", 1
        )
        restored = restored.replace(
            "'design':'Original bed platform and bedding retained; Brambley-style silver faux-leather crystal-button diamond-tufted headboard insert, beveled mirror frame, subtly embossed silver outer bands and recessed perimeter RGB backlight',",
            "'design':'Charcoal channel-upholstered headboard, padded upholstered rails, silver cotton bedding, graphite duvet, midnight ribbed throw',",
            1
        )
        self.assertEqual(restored, self.source)
        for token in ("mattress", "duvet", "throw", "footboard", "platform"):
            with self.subTest(token=token):
                self.assertIn(token, self.patched.lower())

    def test_original_camera_and_controller_code_is_not_patched(self):
        # The patch replaces one exact bed geometry block plus two normal-cache lines
        # and one design metadata string. No tracking, locomotion, or controller code.
        self.assertEqual(self.source.count(OLD_GEOMETRY), 1)
        self.assertIn("Headboard_recessed_LED_channel", NEW_GEOMETRY)
        for token in ("head tracking", "locomotion", "controller", "camera"):
            self.assertNotIn(token, NEW_GEOMETRY.lower())

    def test_smooth_vertex_normals_are_exported(self):
        self.assertIn("m.vertex_normals  # cache smooth per-vertex normals for GLB export", self.patched)
        self.assertIn("merged.vertex_normals  # include normals in optimized runtime batches", self.patched)


if __name__ == "__main__":
    unittest.main(verbosity=2)
