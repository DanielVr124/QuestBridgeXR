from __future__ import annotations
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]

class BlenderBedSourceTests(unittest.TestCase):
    def test_reproducible_model_generator_defines_upholstery_and_bedding_meshes(self):
        generator = (ROOT / "art/bed/generate_bed_asset.py").read_text(encoding="utf-8")
        for token in (
            "PBRMaterial", "TextureVisuals", "superellipsoid", "curve_tube",
            "surface_grid", "Upholstered_platform", "Headboard_channel_",
            "Graphite_duvet_with_soft_folds", "Midnight_blue_ribbed_throw",
            "arc_upholstered_bed_editable.glb", "arc_upholstered_bed.glb",
        ):
            with self.subTest(token=token):
                self.assertIn(token, generator)

    def test_blender_scene_builder_keeps_model_editable_and_renders_studio_view(self):
        blender = (ROOT / "tools/blender_import_bed.py").read_text(encoding="utf-8")
        self.assertIn("arc_upholstered_bed_editable.glb", blender)
        self.assertIn("BED MODEL | editable named parts", blender)
        self.assertIn("STUDIO | render-only setup", blender)
        self.assertIn("bpy.ops.wm.save_as_mainfile", blender)
        self.assertIn("bpy.ops.render.render(write_still=True)", blender)
        self.assertIn("scene.render.engine = 'CYCLES'", blender)

    def test_runtime_room_uses_the_model_instead_of_the_old_procedural_bed(self):
        renderer = (ROOT / "quest-client/app/src/main/cpp/room_renderer.cpp").read_text(encoding="utf-8")
        header = (ROOT / "quest-client/app/src/main/cpp/room_renderer.h").read_text(encoding="utf-8")
        self.assertIn("GlbModel bedModel_", header)
        self.assertIn('bedModel_.load(assets,"models/arc_upholstered_bed.glb")', renderer)
        self.assertIn("bedModel_.draw(pv.data(),bedPlacement", renderer)
        self.assertIn("bedModel_.release()", renderer)
        self.assertNotIn("Headboard_channel_", renderer)

if __name__ == "__main__":
    unittest.main(verbosity=2)
