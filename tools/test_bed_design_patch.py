#!/usr/bin/env python3
"""Regression checks for the Brambley-reference headboard-only redesign."""
from __future__ import annotations
import gzip
import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from bed_design_patch import (OLD_GEOMETRY, NEW_GEOMETRY, IMAGE_IMPORT_OLD,
                              IMAGE_IMPORT_NEW, HEADBOARD_MATERIALS, patch_bed_geometry)


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
            "Headboard_mirror_bevel_",
            "Headboard_inner_shadow_reveal",
            "Headboard_connected_outer_LED_channel",
            "Headboard_connected_outer_LED_strip",
            "Headboard_crystal_tufted_silver_insert",
            "Headboard_crystal_button_setting_",
            "Headboard_crystal_tuft_button_",
            "Headboard_tuft_seam_",
            "Headboard_mount_",
        ):
            with self.subTest(token=token):
                self.assertIn(token, self.patched)

        self.assertIn("(1.075,.685,.135),'headboard_leather'", self.patched)
        self.assertIn("materials['headboard_chrome']", self.patched)
        self.assertIn("materials['headboard_crystal']", self.patched)
        self.assertIn("materials['headboard_seam']", self.patched)
        self.assertIn("materials['headboard_led_channel']", self.patched)
        self.assertIn("materials['headboard_led_diffuser']", self.patched)
        self.assertIn("materials['headboard_emboss_horizontal']", self.patched)
        self.assertIn("materials['headboard_emboss_vertical']", self.patched)
        self.assertIn("headboard_alligator_horizontal", self.patched)
        self.assertIn("tuft_rows = (", self.patched)
        self.assertIn("(-.615,-.205,.205,.615)", self.patched)
        self.assertIn("tuft_y = (1.70,1.43,1.16,.89,.62)", self.patched)
        self.assertIn("def _add_brambley_quilt_diamond", self.patched)
        self.assertIn("Headboard_quilt_diamond_", self.patched)
        self.assertIn("puff=.085,rings=18,segments=72", self.patched)
        self.assertIn("z_edge + puff*((1.0-radius*radius)**2.0)", self.patched)
        self.assertIn("f'Headboard_tuft_seam_{row}_{col}_{next_col}'", self.patched)
        self.assertIn("<= .206", self.patched)
        self.assertIn(".0018,'headboard_seam',sides=8", self.patched)
        self.assertIn("Headboard_crystal_tuft_button_", self.patched)
        self.assertIn("headboard_led_diffuser", self.patched)
        self.assertIn("perimeter_points.append(perimeter_points[0])", self.patched)
        self.assertIn("curve_tube(\n    'Headboard_connected_outer_LED_strip'", self.patched)
        self.assertIn("led_corner_radius = .070", self.patched)
        self.assertIn("recessed perimeter RGB backlight", self.patched)
        self.assertEqual(self.patched.count("Headboard_crystal_tufted_silver_insert"), 1)


    def test_sleep_pillows_are_soft_box_gusset_meshes_not_primitive_ellipsoids(self):
        for token in (
            "Sleep_pillow_top_",
            "Sleep_pillow_box_gusset_",
            "Sleep_pillow_underside_",
            "Sleep_pillow_top_welt_",
            "Sleep_pillow_lower_gusset_seam_",
            "pillow_corner_tucks",
            "PILLOW_CROWN_HEIGHT = .205",
            "PILLOW_CORNER_EXPONENT = 3.8",
            "def pillow_crown_height",
        ):
            with self.subTest(token=token):
                self.assertIn(token, self.patched)
        self.assertNotIn("superellipsoid(f'Sleep_pillow_", self.patched)
        self.assertIn("np.array([side*.50, 1.105, -1.35]", self.patched)
        self.assertEqual(self.patched.count("f'Sleep_pillow_top_{side}'"), 1)
        self.assertIn("PILLOW_RING_ROWS = 32", self.patched)
        self.assertIn("PILLOW_RING_SEGMENTS = 128", self.patched)
        self.assertIn("pillow_corner_tucks", self.patched)
        self.assertIn("fill_lobes", self.patched)
        self.assertNotIn("Sleep_pillow_corner_fold_", self.patched)
        self.assertNotIn("Slate_accent", self.patched)
        self.assertNotIn("Lumbar_cushion", self.patched)

    def test_exactly_two_sleep_pillows_are_propped_against_the_headboard(self):
        self.assertIn("PILLOW_HALF_WIDTH = .475", self.patched)
        self.assertIn("PILLOW_HALF_DEPTH = .33", self.patched)
        self.assertIn("PILLOW_REAR_TILT = math.radians(50.0)", self.patched)
        self.assertIn("PILLOW_REST_PIVOT", self.patched)
        self.assertIn("pillow_rotation[:3, :3] @ (local - PILLOW_REST_PIVOT)", self.patched)
        self.assertIn("for side in (-1, 1):", self.patched)
        self.assertNotIn("Slate_accent", self.patched)
        self.assertNotIn("Lumbar_cushion", self.patched)
        self.assertIn("materials['pillow_cotton']", self.patched)
        self.assertIn("def _make_pillow_cotton_normal_map", self.patched)
        self.assertIn("def _make_pillow_cotton_albedo", self.patched)
        self.assertIn("pillow_cotton_percale.png", self.patched)
        self.assertIn("normalTexture=_pillow_cotton_normal", self.patched)
        self.assertIn("baseColorTexture=_pillow_cotton_albedo", self.patched)
        self.assertNotIn("materials['pillow_gusset']", self.patched)
        self.assertNotIn("materials['pillow_thread']", self.patched)
        self.assertIn("top_welt, .0022, 'linen'", self.patched)

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

    def test_original_bed_body_bedding_and_two_pillow_layout_are_retained(self):
        restored = self.patched.replace(NEW_GEOMETRY, OLD_GEOMETRY, 1)
        restored = restored.replace(
            "        m.vertex_normals  # cache smooth per-vertex normals for GLB export\n", "", 1
        )
        restored = restored.replace(
            "    merged.vertex_normals  # include normals in optimized runtime batches\n", "", 1
        )
        restored = restored.replace(
            "'design':'Original bed platform and bedding retained; Brambley-style pearl-silver faux-leather with ten raised diamond-quilt cells, 23 crystal tuft points, mirror-chrome bevels, embossed silver alligator bands and recessed perimeter RGB backlight',",
            "'design':'Charcoal channel-upholstered headboard, padded upholstered rails, silver cotton bedding, graphite duvet, midnight ribbed throw',",
            1
        )
        restored = restored.replace(HEADBOARD_MATERIALS, "", 1)
        restored = restored.replace(IMAGE_IMPORT_NEW, IMAGE_IMPORT_OLD, 1)
        self.assertEqual(restored, self.source)
        for token in ("mattress", "duvet", "throw", "footboard", "platform"):
            with self.subTest(token=token):
                self.assertIn(token, self.patched.lower())

    def test_original_camera_and_controller_code_is_not_patched(self):
        # The patch replaces one exact bed geometry block plus two normal-cache lines
        # and one design metadata string. No tracking, locomotion, or controller code.
        self.assertEqual(self.source.count(OLD_GEOMETRY), 1)
        self.assertIn("Headboard_connected_outer_LED_strip", NEW_GEOMETRY)
        for token in ("head tracking", "locomotion", "controller", "camera"):
            self.assertNotIn(token, NEW_GEOMETRY.lower())

    def test_smooth_vertex_normals_are_exported(self):
        self.assertIn("m.vertex_normals  # cache smooth per-vertex normals for GLB export", self.patched)
        self.assertIn("merged.vertex_normals  # include normals in optimized runtime batches", self.patched)


if __name__ == "__main__":
    unittest.main(verbosity=2)
