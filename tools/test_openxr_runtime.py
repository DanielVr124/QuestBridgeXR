"""Tests for safe OpenXR manifest discovery and per-process runtime selection."""
from __future__ import annotations

import json
import os
import pathlib
import tempfile
import unittest
from unittest import mock
import openxr_runtime as runtime_module

from openxr_runtime import (
    discover_runtimes,
    launch_with_runtime,
    read_manifest,
    resolve_runtime_manifest,
)


class OpenXRRuntimeTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = pathlib.Path(self.temp.name)

    def write_manifest(self, path: pathlib.Path, name: str = "Fixture Runtime") -> pathlib.Path:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps({"file_format_version": "1.0.0", "runtime": {
            "name": name,
            "library_path": "fixture_runtime.dll"
        }}), encoding="utf-8")
        return path

    def test_read_manifest_validates_schema_and_library(self):
        valid = self.write_manifest(self.root / "valid.json")
        info = read_manifest(valid)
        self.assertFalse(info.error)
        self.assertEqual(info.name, "Fixture Runtime")
        self.assertEqual(info.library_path, "fixture_runtime.dll")

        bad = self.root / "bad.json"
        bad.write_text('{"runtime": {"name": "missing library"}}', encoding="utf-8")
        self.assertIn("library_path", read_manifest(bad).error)
        self.assertIn("No such file", read_manifest(self.root / "missing.json").error)

    def test_meta_runtime_resolution_uses_manifest_path_not_manifest_name(self):
        manifest = self.write_manifest(self.root / "Program Files/Oculus/Support/oculus-runtime/oculus_openxr_64.json", "Oculus")
        env = {"ProgramFiles": str(self.root / "Program Files")}
        found = discover_runtimes(env=env, registry_path=None)
        resolved = resolve_runtime_manifest("meta", found)
        self.assertEqual(resolved, manifest.resolve())

    def test_active_runtime_prefers_environment_override_like_openxr_loader(self):
        env_manifest = self.write_manifest(self.root / "env.json", "Override")
        reg_manifest = self.write_manifest(self.root / "registry.json", "Registry")
        found = discover_runtimes(env={"XR_RUNTIME_JSON": str(env_manifest)}, registry_path=reg_manifest)
        self.assertEqual(resolve_runtime_manifest("active", found), env_manifest.resolve())

    def test_custom_runtime_requires_valid_manifest(self):
        valid = self.write_manifest(self.root / "custom.json")
        self.assertEqual(resolve_runtime_manifest("custom", [], str(valid)), valid.resolve())
        with self.assertRaisesRegex(ValueError, "required"):
            resolve_runtime_manifest("custom", [])
        with self.assertRaisesRegex(ValueError, "Invalid runtime manifest"):
            resolve_runtime_manifest("custom", [], str(self.root / "not-found.json"))

    def test_child_launch_sets_only_child_environment_and_does_not_change_parent(self):
        app = self.root / "app.exe"
        app.write_bytes(b"fixture")
        manifest = self.write_manifest(self.root / "runtime.json")
        before = os.environ.get("XR_RUNTIME_JSON")
        called = {}

        class Child:
            pid = 1234

        def fake_popen(command, *, cwd, env):
            called.update(command=command, cwd=cwd, env=env)
            return Child()

        child = launch_with_runtime(app, ["--demo", "value"], manifest, popen=fake_popen)
        self.assertEqual(child.pid, 1234)
        self.assertEqual(called["command"], [str(app.resolve()), "--demo", "value"])
        self.assertEqual(called["env"]["XR_RUNTIME_JSON"], str(manifest.resolve()))
        self.assertEqual(os.environ.get("XR_RUNTIME_JSON"), before)

    def test_launch_refuses_missing_application(self):
        manifest = self.write_manifest(self.root / "runtime.json")
        with self.assertRaises(FileNotFoundError):
            launch_with_runtime(self.root / "missing.exe", [], manifest, popen=lambda *a, **k: None)

    def test_discovery_deduplicates_manifest_paths(self):
        manifest = self.write_manifest(self.root / "same.json")
        found = discover_runtimes(env={"XR_RUNTIME_JSON": str(manifest)}, registry_path=manifest)
        valid = [item for item in found if not item.error]
        self.assertEqual(len(valid), 1)

    def test_qbx_mode_resolves_only_a_built_bundled_runtime(self):
        manifest = self.write_manifest(self.root / "runtime" / "questbridge_openxr_runtime.json", "QuestBridge XR Experimental Runtime")
        (manifest.parent / "fixture_runtime.dll").write_bytes(b"runtime")
        with mock.patch.object(runtime_module, "bundled_qbx_runtime_manifest", return_value=manifest.resolve()):
            self.assertEqual(resolve_runtime_manifest("qbx", []), manifest.resolve())

    def test_qbx_mode_errors_if_runtime_not_built(self):
        with mock.patch.object(runtime_module, "bundled_qbx_runtime_manifest", side_effect=FileNotFoundError("DLL is missing")):
            with self.assertRaisesRegex(FileNotFoundError, "DLL is missing"):
                resolve_runtime_manifest("qbx", [])


if __name__ == "__main__":
    unittest.main(verbosity=2)
