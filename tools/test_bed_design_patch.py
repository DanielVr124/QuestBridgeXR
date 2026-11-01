#!/usr/bin/env python3
"""Regression checks for the bed-only headboard and pillow restoration."""
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

    def test_headboard_uses_five_softly_eased_filled_panels(self):
        for token in (
            "Headboard_back_shell",
            "Headboard_quilt_backing",
            "Headboard_filled_panel_",
            "Headboard_outer_lip_side_",
            "Headboard_inner_lip_side_",
            "Headboard_outer_lip_edge_",
            "Headboard_inner_lip_edge_",
            "Headboard_mount_",
        ):
            with self.subTest(token=token):
                self.assertIn(token, self.patched)
        self.assertIn("(.225,.705,.112)", self.patched)
        self.assertIn("taper=.10", self.patched)
        self.assertIn("nested recessed RGB perimeter channel", self.patched)

    def test_old_bed_base_and_bedding_are_not_replaced(self):
        # The patch is an exact replacement of the original pillow/headboard block.
        restored = self.patched.replace(NEW_GEOMETRY, OLD_GEOMETRY, 1)
        restored = restored.replace(
            "        m.vertex_normals  # cache smooth per-vertex normals for GLB export\n", "", 1
        )
        restored = restored.replace(
            "    merged.vertex_normals  # include normals in optimized runtime batches\n", "", 1
        )
        restored = restored.replace(
            "'design':'Original bed platform, frame, mattress and bedding retained; five softly padded charcoal headboard sections with gently eased rectangular edges, a nested recessed RGB perimeter channel and refreshed fabric pillows',",
            "'design':'Charcoal channel-upholstered headboard, padded upholstered rails, silver cotton bedding, graphite duvet, midnight ribbed throw',",
            1
        )
        self.assertEqual(restored, self.source)
        for token in ("mattress", "duvet", "throw", "footboard", "platform"):
            with self.subTest(token=token):
                self.assertIn(token, self.patched.lower())

    def test_only_pillows_and_headboard_change_inside_geometry_block(self):
        for token in (
            "Sleep_pillow_", "Sleep_pillow_piping_", "Sleep_pillow_corner_fold_",
            "Slate_accent_cushion_", "Slate_accent_piping_",
            "Lumbar_cushion", "Lumbar_cushion_seam",
        ):
            with self.subTest(token=token):
                self.assertIn(token, NEW_GEOMETRY)
        self.assertIn("bed platform, rails, legs, foot end", NEW_GEOMETRY.lower())
        self.assertIn("mattress, fitted sheet", NEW_GEOMETRY)
        self.assertIn("duvet and throw untouched", NEW_GEOMETRY)

    def test_previous_standalone_single_panel_and_diamond_button_design_are_absent(self):
        for token in (
            "Headboard_single_upholstered_panel",
            "Headboard_diamond_tuft_",
            "Headboard_tuft_button_",
            "Headboard_crystal_tuft_button_",
            "Headboard_channel_",
            "Headboard_LED_strip",
        ):
            with self.subTest(token=token):
                self.assertNotIn(token, self.patched)

    def test_smooth_vertex_normals_are_exported(self):
        self.assertIn("m.vertex_normals  # cache smooth per-vertex normals for GLB export", self.patched)
        self.assertIn("merged.vertex_normals  # include normals in optimized runtime batches", self.patched)


if __name__ == "__main__":
    unittest.main(verbosity=2)
