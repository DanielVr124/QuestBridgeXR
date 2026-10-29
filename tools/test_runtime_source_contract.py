"""Static contract tests for the experimental native OpenXR runtime build."""
from __future__ import annotations

import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
RUNTIME = ROOT / "runtime"


class RuntimeSourceContractTests(unittest.TestCase):
    def test_runtime_has_loader_negotiation_entrypoint_and_d3d11_surface(self):
        source = (RUNTIME / "src" / "qbx_runtime.cpp").read_text(encoding="utf-8")
        for token in (
            'xrNegotiateLoaderRuntimeInterface',
            'qbxGetInstanceProcAddr',
            '"xrCreateInstance"',
            '"xrCreateSession"',
            '"xrCreateSwapchain"',
            '"xrLocateViews"',
            '"xrEnumerateInstanceVersion"',
            'ActionStateCache',
            'ActionHasBindingForHand',
            'XR_ERROR_API_LAYER_NOT_PRESENT',
            'XR_ERROR_ACTIONSETS_ALREADY_ATTACHED',
            'XR_ERROR_ACTIONSET_NOT_ATTACHED',
            'XR_ERROR_NAME_DUPLICATED',
            'XR_ACTION_TYPE_VIBRATION_OUTPUT',
            'XR_VERSION_MINOR(info->applicationInfo.apiVersion) > 0',
            'state->changedSinceLastSync = c.changed ? XR_TRUE : XR_FALSE',
            '"xrWaitFrame"',
            '"xrBeginFrame"',
            '"xrEndFrame"',
            'MirrorSubmittedFrame',
            'D3DCompile',
            'std::find(instance->enabledExtensions.begin()',
            'kTelemetryPort = 9945',
            'maxLayerCount = 1',
            'XR_ERROR_LAYER_LIMIT_EXCEEDED',
            'XR_ERROR_ENVIRONMENT_BLEND_MODE_UNSUPPORTED',
        ):
            with self.subTest(token=token):
                self.assertIn(token, source)

    def test_runtime_exports_only_loader_negotiation_directly(self):
        source = (RUNTIME / "src" / "qbx_runtime.cpp").read_text(encoding="utf-8")
        exports = [line for line in source.splitlines() if "__declspec(dllexport)" in line]
        self.assertEqual(len(exports), 1)
        self.assertIn("xrNegotiateLoaderRuntimeInterface", exports[0])

    def test_runtime_manifest_is_relative_and_names_the_dll(self):
        manifest = json.loads((RUNTIME / "manifest" / "questbridge_openxr_runtime.json").read_text(encoding="utf-8"))
        self.assertEqual(manifest["file_format_version"], "1.0.0")
        runtime = manifest["runtime"]
        self.assertIn("experimental", runtime["name"].casefold())
        self.assertEqual(runtime["library_path"], "./qbx_openxr_runtime.dll")
        self.assertFalse(Path(runtime["library_path"]).is_absolute())

    def test_windows_builder_compiles_and_copies_runtime_dll_manifest(self):
        script = (ROOT / "scripts" / "build-portable.ps1").read_text(encoding="utf-8")
        self.assertIn("-S (Join-Path $root 'runtime')", script)
        self.assertIn("qbx_openxr_runtime.dll", script)
        self.assertIn("questbridge_openxr_runtime.json", script)
        cmake = (RUNTIME / "CMakeLists.txt").read_text(encoding="utf-8")
        self.assertIn("release-1.1.63", cmake)
        self.assertIn("d3dcompiler", cmake)

    def test_workflow_requires_runtime_dll_manifest_and_discloses_status(self):
        workflow = (ROOT / ".github" / "workflows" / "build-release.yml").read_text(encoding="utf-8")
        self.assertIn("runtime/qbx_openxr_runtime.dll", workflow)
        self.assertIn("runtime/questbridge_openxr_runtime.json", workflow)
        self.assertIn("not Khronos-conformance-tested", workflow)

    def test_per_process_helper_has_explicit_qbx_mode_without_registry_mutation(self):
        helper = (ROOT / "tools" / "openxr_runtime.py").read_text(encoding="utf-8")
        self.assertIn('if mode == "qbx"', helper)
        self.assertIn('"qbx"', helper)
        self.assertNotIn("winreg.SetValue", helper)

    def test_studio_has_explicit_experimental_runtime_launch(self):
        studio = (ROOT / "tools" / "questbridge_studio.py").read_text(encoding="utf-8")
        self.assertIn("Launch an EXE with QuestBridge runtime (experimental)", studio)
        self.assertIn('self._launch_openxr_app("qbx")', studio)


if __name__ == "__main__":
    unittest.main(verbosity=2)
