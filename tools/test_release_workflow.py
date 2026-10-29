"""Static contract tests for the reproducible Windows + Quest release workflow."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]

class ReleaseWorkflowTests(unittest.TestCase):
    def setUp(self):
        self.workflow = (ROOT / ".github/workflows/build-release.yml").read_text(encoding="utf-8")

    def test_tag_push_triggers_workflow(self):
        self.assertIn('tags: ["v*"]', self.workflow)

    def test_windows_outputs_are_verified_before_upload(self):
        for exe in ("QuestBridgeStudio.exe", "QuestBridgeHost.exe", "doctor.exe", "stream_test_video.exe", "monitor_telemetry.exe", "openxr_runtime.exe", "qbx_openxr_runtime.dll"):
            self.assertIn(exe, self.workflow)
        self.assertIn("runtime/questbridge_openxr_runtime.json", self.workflow)

    def test_quest_apk_is_validated_and_combined(self):
        self.assertIn("unzip -t", self.workflow)
        self.assertIn("QuestBridgeXR-Quest-debug.apk", self.workflow)
        self.assertIn("QuestBridgeXR-Portable", self.workflow)
        self.assertIn("innosetup", self.workflow.casefold())
        self.assertIn("QuestBridgeXR-*-Setup.exe", self.workflow)
        installer = (ROOT / "installer/QuestBridgeXR.iss").read_text(encoding="utf-8")
        self.assertIn("PrivilegesRequired=lowest", installer)
        self.assertIn("uninsneveruninstall", installer)
        self.assertIn('Excludes: "config\\questbridge.json;config\\profiles\\*.json"', installer)

    def test_tagged_release_publishes_zip_and_checksum(self):
        self.assertIn("publish-release:", self.workflow)
        self.assertIn("softprops/action-gh-release@v2", self.workflow)
        self.assertIn("contents: write", self.workflow)
        self.assertIn("*.sha256", self.workflow)

    def test_host_uses_static_msvc_runtime_for_portable_release(self):
        cmake = (ROOT / "host/CMakeLists.txt").read_text(encoding="utf-8")
        self.assertIn("MSVC_RUNTIME_LIBRARY", cmake)

    def test_quest_manifest_declares_immersive_mode(self):
        manifest = (ROOT / "quest-client/app/src/main/AndroidManifest.xml").read_text(encoding="utf-8")
        self.assertIn('com.oculus.vr.application.mode', manifest)
        self.assertIn('android:value="vr"', manifest)

if __name__ == "__main__":
    unittest.main()
