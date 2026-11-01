#!/usr/bin/env python3
"""Tests for Quest USB/ADB connection diagnostics."""
from __future__ import annotations

import pathlib
import sys
import unittest
from unittest import mock

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from usb_connection import find_adb_executable, parse_adb_devices, probe_usb_connection


class UsbConnectionTests(unittest.TestCase):
    def test_parse_adb_devices_classifies_usb_network_and_emulator(self):
        output = """List of devices attached
1WMHH123456789 device product:eureka model:Quest_3 transport_id:7
192.168.1.22:5555 device product:eureka model:Quest_3 transport_id:8
emulator-5554 device product:sdk_gphone64_x86_64 transport_id:9
"""
        devices = parse_adb_devices(output)
        self.assertEqual([(d.serial, d.transport, d.state) for d in devices], [
            ("1WMHH123456789", "usb", "device"),
            ("192.168.1.22:5555", "network", "device"),
            ("emulator-5554", "emulator", "device"),
        ])

    def test_parse_adb_devices_keeps_unauthorized_usb_state(self):
        devices = parse_adb_devices("List of devices attached\n1WMHH123 unauthorized usb:1-2\n")
        self.assertEqual(len(devices), 1)
        self.assertEqual(devices[0].transport, "usb")
        self.assertEqual(devices[0].state, "unauthorized")

    def test_find_adb_checks_portable_platform_tools(self):
        with __import__("tempfile").TemporaryDirectory() as temp:
            root = pathlib.Path(temp)
            tools = root / "platform-tools"
            tools.mkdir()
            adb = tools / ("adb.exe" if __import__("os").name == "nt" else "adb")
            adb.write_text("test", encoding="utf-8")
            with mock.patch.dict("usb_connection.os.environ", {}, clear=True):
                with mock.patch("usb_connection.shutil.which", return_value=None):
                    found = find_adb_executable(root)
            self.assertEqual(pathlib.Path(found), adb.resolve())

    def test_no_adb_returns_actionable_failure(self):
        with mock.patch("usb_connection.find_adb_executable", return_value=None):
            report = probe_usb_connection(project_root=ROOT)
        self.assertFalse(report["ok"])
        self.assertEqual(report["checks"][0]["level"], "FAIL")
        self.assertIn("Platform-Tools", report["summary"])

    @mock.patch("usb_connection.subprocess.run")
    def test_unauthorized_headset_is_not_reported_as_connected(self, run):
        run.return_value = mock.Mock(returncode=0, stdout=(
            "List of devices attached\n1WMHH123 unauthorized usb:1-2 product:eureka model:Quest_3\n"
        ), stderr="")
        with mock.patch("usb_connection.pathlib.Path.is_file", return_value=True):
            report = probe_usb_connection(adb_path="C:/Android/adb.exe")
        self.assertFalse(report["ok"])
        self.assertIn("authorization", report["summary"].lower())
        self.assertEqual(run.call_count, 1)

    @mock.patch("usb_connection.subprocess.run")
    def test_non_quest_android_device_is_not_reported_as_quest_ready(self, run):
        outputs = [
            (0, "List of devices attached\nR58M123456 device product:generic model:Android_Phone transport_id:7\n"),
            (0, "Android Phone\n"),
            (0, "14\n"),
        ]
        run.side_effect = [
            mock.Mock(returncode=code, stdout=stdout, stderr="")
            for code, stdout in outputs
        ]
        with mock.patch("usb_connection.pathlib.Path.is_file", return_value=True):
            report = probe_usb_connection(adb_path="C:/Android/adb.exe")
        self.assertFalse(report["ok"])
        self.assertIn("not identified as a Meta Quest", report["summary"])
        self.assertEqual(run.call_count, 3)

    @mock.patch("usb_connection.subprocess.run")
    def test_usb_quest_with_installed_running_app_passes(self, run):
        outputs = [
            (0, "List of devices attached\n1WMHH123 device product:eureka model:Quest_3 transport_id:7\n"),
            (0, "Quest 3\n"),
            (0, "14\n"),
            (0, "package:/data/app/com.questbridge.xr/base.apk\n"),
            (0, "12345\n"),
            (0, "Windows host accepted connection (protocol=1 capabilities=0x0000000f)\nReceived video config 1920x1080 @72 codec=1 layout=0\nVideo decoder configured: video/avc 1920x1080\n"),
        ]
        run.side_effect = [
            mock.Mock(returncode=code, stdout=stdout, stderr="")
            for code, stdout in outputs
        ]
        with mock.patch("usb_connection.pathlib.Path.is_file", return_value=True):
            report = probe_usb_connection(adb_path="C:/Android/adb.exe")
        self.assertTrue(report["ok"])
        self.assertTrue(report["app_installed"])
        self.assertTrue(report["app_running"])
        self.assertTrue(report["host_handshake_seen"])
        self.assertTrue(report["video_config_seen"])
        self.assertIn("Quest 3", report["summary"])
        self.assertEqual(run.call_count, 6)

    @mock.patch("usb_connection.subprocess.run")
    def test_usb_quest_reports_wired_tcp_handshake(self, run):
        outputs = [
            (0, "List of devices attached\\n1WMHH123 device product:eureka model:Quest_3 transport_id:7\\n"),
            (0, "Quest 3\\n"),
            (0, "14\\n"),
            (0, "package:/data/app/com.questbridge.xr/base.apk\\n"),
            (0, "12345\\n"),
            (0, "Wired USB stream handshake accepted (protocol=1 capabilities=0x0000000f)\\n"
                "Received video config 1920x1080 @72 codec=1 layout=0\\n"
                "Video decoder configured: video/avc 1920x1080\\n"),
        ]
        run.side_effect = [
            mock.Mock(returncode=code, stdout=stdout, stderr="")
            for code, stdout in outputs
        ]
        with mock.patch("usb_connection.pathlib.Path.is_file", return_value=True):
            report = probe_usb_connection(adb_path="C:/Android/adb.exe")
        self.assertTrue(report["ok"])
        self.assertTrue(report["host_handshake_seen"])
        self.assertTrue(report["video_config_seen"])


if __name__ == "__main__":
    unittest.main(verbosity=2)
