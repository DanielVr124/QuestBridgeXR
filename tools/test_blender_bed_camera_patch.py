#!/usr/bin/env python3
"""Regression tests for the Blender bed review-camera patch."""
from __future__ import annotations

import gzip
import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from blender_bed_camera_patch import patch_blender_review_cameras


class BlenderBedCameraPatchTests(unittest.TestCase):
    def test_replaces_review_views_even_when_original_coordinates_differ(self):
        source = """scene = True
review_views = [
    ('bed_three_quarter', (9.1, 4.2, 7.8), (0, 1, 0), 40),
    ('bed_headboard', (2.0, 3.0, 5.0), (0, 1, -1), 60),
    ('bed_side_profile', (6.0, 2.0, 0.0), (0, 1, 0), 45),
    ('bed_foot_three_quarter', (-5.0, 4.0, 6.0), (0, 1, 0), 42),
]
camera_count = len(review_views)
"""
        patched = patch_blender_review_cameras(source)
        self.assertEqual(patched.count("review_views = ["), 1)
        self.assertIn("bed_pillow_detail", patched)
        self.assertIn("(5.5,-4.3,4.0)", patched)
        self.assertIn("camera_count = len(review_views)", patched)
        self.assertNotIn("(9.1, 4.2, 7.8)", patched)

    def test_patches_the_actual_gzip_packed_blender_scene(self):
        packed_scene = ROOT / "tools" / "blender_import_bed.py.gz"
        source = gzip.decompress(packed_scene.read_bytes()).decode("utf-8")
        patched = patch_blender_review_cameras(source)
        self.assertIn("bed_pillow_detail", patched)
        self.assertIn("(3.3,4.25,2.8)", patched)
        self.assertEqual(patched.count("review_views = ["), 1)

    def test_refuses_a_scene_without_the_expected_camera_list(self):
        with self.assertRaisesRegex(RuntimeError, "expected a review_views list"):
            patch_blender_review_cameras("scene = True\n")

    def test_refuses_an_unrelated_review_views_list(self):
        with self.assertRaisesRegex(RuntimeError, "required bed camera entries"):
            patch_blender_review_cameras("review_views = [('other', (0,0,0), (0,0,0), 50)]\n")


if __name__ == "__main__":
    unittest.main(verbosity=2)
