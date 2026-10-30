#!/usr/bin/env python3
"""Validate bundled mood-reference assets used by the 3D Quest room."""
from __future__ import annotations
import pathlib
import struct
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
ASSET_DIR = ROOT / "quest-client" / "app" / "src" / "main" / "assets" / "environments"
MATERIAL_DIR = ROOT / "quest-client" / "app" / "src" / "main" / "assets" / "materials"

class RoomAssetTests(unittest.TestCase):
    def test_each_environment_asset_has_valid_rgb_header_and_payload(self):
        expected = {"blossom.rgb", "night_city.rgb", "golden_city.rgb"}
        self.assertEqual({p.name for p in ASSET_DIR.glob("*.rgb")}, expected)
        for name in sorted(expected):
            with self.subTest(asset=name):
                raw = (ASSET_DIR / name).read_bytes()
                self.assertGreaterEqual(len(raw), 8)
                width, height = struct.unpack("<II", raw[:8])
                self.assertGreaterEqual(width, 2)
                self.assertGreaterEqual(height, 2)
                self.assertLessEqual(width, 2048)
                self.assertLessEqual(height, 2048)
                self.assertEqual(len(raw), 8 + width * height * 3)

    def test_room_material_atlases_have_valid_rgb_headers_and_dimensions(self):
        expected = {
            "room_surface_atlas.rgb": (256, 256),
            "room_normal_atlas.rgb": (256, 256),
            "room_roughness_atlas.rgb": (128, 128),
        }
        self.assertEqual({p.name for p in MATERIAL_DIR.glob("*.rgb")}, set(expected))
        for name, dimensions in expected.items():
            with self.subTest(asset=name):
                raw = (MATERIAL_DIR / name).read_bytes()
                self.assertGreaterEqual(len(raw), 8)
                width, height = struct.unpack("<II", raw[:8])
                self.assertEqual((width, height), dimensions)
                self.assertLessEqual(width, 2048)
                self.assertLessEqual(height, 2048)
                self.assertEqual(len(raw), 8 + width * height * 3)

if __name__ == "__main__":
    unittest.main(verbosity=2)
