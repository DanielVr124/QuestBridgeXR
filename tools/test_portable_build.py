from __future__ import annotations
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]

class PortableBuildTests(unittest.TestCase):
    def test_portable_builder_builds_host_and_bundles_helpers(self):
        script = (ROOT / "scripts" / "build-portable.ps1").read_text(encoding="utf-8")
        self.assertIn("cmake --build", script)
        self.assertIn("QuestBridgeHost.exe", script)
        self.assertIn("python -m PyInstaller", script)
        self.assertIn("QuestBridgeStudio", script)
        self.assertIn("stream_test_video.py", script)
        self.assertIn("openxr_runtime.py", script)
        self.assertIn("qbx_openxr_runtime.dll", script)
        self.assertIn("-S (Join-Path $root 'runtime')", script)

    def test_ci_builds_windows_and_android_products(self):
        workflow = (ROOT / ".github" / "workflows" / "build-release.yml").read_text(encoding="utf-8")
        self.assertIn("windows-2022", workflow)
        self.assertIn("assembleDebug", workflow)
        self.assertIn("QuestBridgeXR-Portable", workflow)
        self.assertIn("QuestBridgeXR-*-Setup.exe", workflow)

    def test_studio_exposes_install_and_launch_apk_action(self):
        studio = (ROOT / "tools" / "questbridge_studio.py").read_text(encoding="utf-8")
        self.assertIn("Install & Launch Quest APK", studio)
        self.assertIn("def _install_quest_app", studio)
        self.assertIn('"monkey", "-p", "com.questbridge.xr", "1"', studio)

    def test_studio_exposes_process_local_openxr_runtime_launch(self):
        studio = (ROOT / "tools" / "questbridge_studio.py").read_text(encoding="utf-8")
        helper = (ROOT / "tools" / "openxr_runtime.py").read_text(encoding="utf-8")
        self.assertIn("Inspect installed OpenXR runtimes", studio)
        self.assertIn("Launch an EXE with Meta runtime", studio)
        self.assertIn("launch_with_runtime", studio)
        self.assertIn('env["XR_RUNTIME_JSON"] = str(info.manifest)', helper)
        self.assertNotIn("winreg.SetValue", helper)

if __name__ == "__main__":
    unittest.main(verbosity=2)
