#!/usr/bin/env python3
"""Static integration guards for native/UI paths that cannot be compiled in this container."""
from __future__ import annotations
import pathlib
import unittest
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from qbx_config import ROOM_ENVIRONMENTS

class NativeSourceContractTests(unittest.TestCase):
    def test_room_renderer_contains_requested_environment_and_seating_paths(self):
        header = (ROOT / "quest-client/app/src/main/cpp/room_renderer.h").read_text(encoding="utf-8")
        source = (ROOT / "quest-client/app/src/main/cpp/room_renderer.cpp").read_text(encoding="utf-8")
        for env in ("blossom", "night_city", "golden_city"):
            with self.subTest(environment=env):
                self.assertIn(env, source)
                self.assertIn(f"environments/{env}.rgb", source)
        self.assertIn("void toggleSitting(const XrPosef& headPose)", header)
        self.assertIn("aurora", source)
        self.assertIn("cyberpunk", source)
        self.assertIn("createProceduralEnvironment(activeEnvironmentIndex_)", source)
        self.assertIn("std::array<const char*, 208> kEnvironmentNames", source)
        self.assertIn("environmentTextures_[activeEnvironmentIndex_]", source)
        catalog_start = source.index("kEnvironmentNames")
        catalog_end = source.index("};", catalog_start)
        native_catalog = source[catalog_start:catalog_end]
        for env in ROOM_ENVIRONMENTS:
            with self.subTest(native_environment=env):
                self.assertIn(f'"{env}"', native_catalog)
        self.assertIn("environmentTextures_.fill(0)", source)
        self.assertIn("glDeleteTextures(1, &environmentTextures_[previousIndex]);", source)
        self.assertIn("Move close to the desk chair or couch", source)
        self.assertIn("Sat on the virtual", source)
        self.assertIn("settings_.showCouch", source)
        self.assertIn("settings_.showChair", source)

    def test_room_controls_keep_locomotion_but_have_no_hidden_click_shortcuts(self):
        source = (ROOT / "quest-client/app/src/main/cpp/main.cpp").read_text(encoding="utf-8")
        self.assertIn("room_.updateLocomotion(moveX, moveY, turnX, dt, headPose)", source)
        self.assertNotIn("room_.cycleScreenDestination()", source)
        self.assertNotIn("room_.cycleEnvironment()", source)
        self.assertNotIn("room_.toggleSitting(headPose)", source)
        self.assertIn('bind(hands_[0].menu, "/user/hand/left/input/menu/click")', source)
        self.assertIn("left Y is a deliberate in-app fallback", source)
        self.assertIn("if (menuPressed && !previousMenuPressed_) room_.toggleMenu()", source)
        self.assertIn("config_.invertThumbstickY ? -move.y : move.y", source)
        self.assertIn("room_.handleMenuInput(move.y, menuSelectEdge)", source)
        renderer = (ROOT / "quest-client/app/src/main/cpp/room_renderer.cpp").read_text(encoding="utf-8")
        self.assertIn("void RoomRenderer::toggleMenu()", renderer)
        self.assertIn("if (menuVisible_)", renderer)
        self.assertIn("Monitor-anchored menu", renderer)
        self.assertIn("void RoomRenderer::handleMenuInput", renderer)
        self.assertIn("drawText(pv.data(), labels[i]", renderer)
        self.assertIn("kEnvironmentNames[activeEnvironmentIndex_]", renderer)
        self.assertIn("yawPivotX_ = headPose.position.x + moveX_ + worldOffsetX_", renderer)

    def test_fake_vr_bridge_is_connected_and_default_off(self):
        host = (ROOT / "host/src/main.cpp").read_text(encoding="utf-8")
        config = (ROOT / "config/fake_vr.ini").read_text(encoding="utf-8")
        bridge = (ROOT / "host/src/fake_vr_bridge.cpp").read_text(encoding="utf-8")
        self.assertIn("g_fakeVr.onTracking(tracking)", host)
        self.assertIn("enabled=false", config)
        self.assertIn("foregroundProcessAllowed()", bridge)
        self.assertIn("SendInput(1", bridge)
        self.assertNotIn("WriteProcessMemory", bridge)

    def test_studio_offers_separate_real_and_fake_play_modes(self):
        studio = (ROOT / "tools/questbridge_studio.py").read_text(encoding="utf-8")
        self.assertIn('self._add_play_modes_tab(notebook)', studio)
        self.assertIn('text="Launch Meta Quest Link"', studio)
        self.assertIn('text="Open Roblox DOORS"', studio)
        self.assertIn('"https://www.roblox.com/games/6516141723/DOORS"', studio)

    def test_play_modes_include_per_process_openxr_helper(self):
        studio = (ROOT / "tools/questbridge_studio.py").read_text(encoding="utf-8")
        helper = (ROOT / "tools/openxr_runtime.py").read_text(encoding="utf-8")
        self.assertIn('text="Launch an EXE with Meta runtime…"', studio)
        self.assertIn('text="Inspect installed OpenXR runtimes"', studio)
        self.assertIn('"XR_RUNTIME_JSON"', helper)
        self.assertIn("registry_runtime_manifest", helper)

if __name__ == "__main__":
    unittest.main(verbosity=2)
