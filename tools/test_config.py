#!/usr/bin/env python3
"""Standard-library tests for settings validation, profiles, and Quest INI export."""
from __future__ import annotations

import copy
import json
import pathlib
import tempfile
import unittest

import qbx_config


class ConfigTests(unittest.TestCase):
    def test_default_config_validates(self):
        cfg = copy.deepcopy(qbx_config.DEFAULT_CONFIG)
        self.assertIs(qbx_config.validate_config(cfg), cfg)

    def test_profiles_are_valid_and_have_distinct_tradeoffs(self):
        balanced = qbx_config.apply_profile("Balanced")
        low = qbx_config.apply_profile("Low Latency")
        quality = qbx_config.apply_profile("High Quality")
        self.assertLess(low["stream"]["width"], quality["stream"]["width"])
        self.assertLess(low["stream"]["bitrate_bps"], quality["stream"]["bitrate_bps"])
        self.assertEqual(balanced["profile"], "Balanced")
        for profile in (balanced, low, quality, qbx_config.apply_profile("Network Debug")):
            qbx_config.validate_config(profile)

    def test_invalid_odd_sbs_width_rejected(self):
        cfg = copy.deepcopy(qbx_config.DEFAULT_CONFIG)
        cfg["stream"]["layout"] = "sbs"
        cfg["stream"]["width"] = 1279
        with self.assertRaisesRegex(ValueError, "even for side-by-side"):
            qbx_config.validate_config(cfg)

    def test_invalid_address_rejected(self):
        cfg = copy.deepcopy(qbx_config.DEFAULT_CONFIG)
        cfg["network"]["quest_ip"] = "not-an-ip"
        with self.assertRaisesRegex(ValueError, "network.quest_ip"):
            qbx_config.validate_config(cfg)

    def test_unsupported_video_encoder_rejected(self):
        cfg = copy.deepcopy(qbx_config.DEFAULT_CONFIG)
        cfg["stream"]["encoder"] = "hevc_nvenc"
        with self.assertRaisesRegex(ValueError, "currently supports H.264 Annex-B only"):
            qbx_config.validate_config(cfg)

    def test_save_load_roundtrip(self):
        cfg = qbx_config.apply_profile("Low Latency")
        cfg["network"]["quest_ip"] = "10.0.0.42"
        with tempfile.TemporaryDirectory() as tmp:
            path = pathlib.Path(tmp) / "test.json"
            qbx_config.save_config(cfg, path)
            loaded = qbx_config.load_config(path)
        self.assertEqual(loaded["network"]["quest_ip"], "10.0.0.42")
        self.assertEqual(loaded["stream"]["fps"], 72)
        self.assertEqual(loaded["profile"], "Low Latency")

    def test_telemetry_port_rejects_host_port_collision(self):
        cfg = copy.deepcopy(qbx_config.DEFAULT_CONFIG)
        cfg["network"]["telemetry_port"] = cfg["network"]["host_port"]
        with self.assertRaisesRegex(ValueError, "must differ"):
            qbx_config.validate_config(cfg)

    def test_telemetry_port_zero_disables_local_feed(self):
        cfg = copy.deepcopy(qbx_config.DEFAULT_CONFIG)
        cfg["network"]["telemetry_port"] = 0
        qbx_config.validate_config(cfg)

    def test_malformed_nested_section_fails_as_value_error(self):
        cfg = copy.deepcopy(qbx_config.DEFAULT_CONFIG)
        cfg["network"] = "not an object"
        with self.assertRaisesRegex(ValueError, "network must be a JSON object"):
            qbx_config.validate_config(cfg)

    def test_missing_required_field_is_reported(self):
        cfg = copy.deepcopy(qbx_config.DEFAULT_CONFIG)
        del cfg["network"]["telemetry_port"]
        with self.assertRaisesRegex(ValueError, "network.telemetry_port is required"):
            qbx_config.validate_config(cfg)

    def test_quest_ini_contains_client_controls(self):
        cfg = qbx_config.apply_profile("Balanced")
        cfg["network"]["pc_ip"] = "10.0.0.5"
        with tempfile.TemporaryDirectory() as tmp:
            path = pathlib.Path(tmp) / "config.ini"
            qbx_config.save_quest_ini(cfg, path)
            text = path.read_text(encoding="utf-8")
        self.assertIn("host_ip=10.0.0.5", text)
        self.assertIn("tracking_hz=72", text)
        self.assertIn("thumbstick_deadzone=0.120", text)
        self.assertIn("invert_thumbstick_x=false", text)
        self.assertIn("controller_pose=grip", text)
        self.assertIn("reference_space=local", text)
        self.assertIn("brightness=0.000", text)
        self.assertIn("swap_eyes=false", text)
        self.assertIn("incomplete_frame_timeout_ms=120", text)
        self.assertIn("saturation=1.000", text)
        self.assertIn("aspect_mode=stretch", text)
        self.assertIn("trigger_curve=1.000", text)
        self.assertIn("quest_receive_buffer_kib=4096", text)

    def test_advanced_profiles_validate_and_apply_tunings(self):
        names = qbx_config.profile_names()
        for name in ("Ultra Low Latency", "WiFi Stable", "Cinema Quality", "Comfort Accessibility"):
            self.assertIn(name, names)
            qbx_config.validate_config(qbx_config.apply_profile(name))
        low = qbx_config.apply_profile("Ultra Low Latency")
        self.assertEqual(low["stream"]["rate_control"], "cbr")
        self.assertEqual(low["stream"]["fps"], 90)
        stable = qbx_config.apply_profile("WiFi Stable")
        self.assertEqual(stable["transport"]["quest_receive_buffer_kib"], 6144)
        quality = qbx_config.apply_profile("Cinema Quality")
        self.assertEqual(quality["display"]["aspect_mode"], "fit")

    def test_new_tuning_ranges_are_validated(self):
        cfg = copy.deepcopy(qbx_config.DEFAULT_CONFIG)
        cfg["stream"]["vbv_buffer_ms"] = 0
        with self.assertRaisesRegex(ValueError, "stream.vbv_buffer_ms"):
            qbx_config.validate_config(cfg)
        cfg = copy.deepcopy(qbx_config.DEFAULT_CONFIG)
        cfg["display"]["aspect_mode"] = "warp"
        with self.assertRaisesRegex(ValueError, "display.aspect_mode"):
            qbx_config.validate_config(cfg)
        cfg = copy.deepcopy(qbx_config.DEFAULT_CONFIG)
        cfg["tracking"]["trigger_curve"] = 5.0
        with self.assertRaisesRegex(ValueError, "tracking.trigger_curve"):
            qbx_config.validate_config(cfg)

    def test_room_settings_validate_and_export_to_quest(self):
        cfg = copy.deepcopy(qbx_config.DEFAULT_CONFIG)
        cfg["room"].update({
            "environment": "golden_city",
            "room_size": "large",
            "screen_destination": "both",
            "monitor_count": 3,
            "movement_enabled": True,
            "movement_speed": 2.0,
            "smooth_turn": True,
        })
        qbx_config.validate_config(cfg)
        with tempfile.TemporaryDirectory() as tmp:
            path = pathlib.Path(tmp) / "config.ini"
            qbx_config.save_quest_ini(cfg, path)
            text = path.read_text(encoding="utf-8")
        for expected in (
            "environment=golden_city", "room_size=large", "screen_destination=both",
            "monitor_count=3", "room_movement_enabled=true", "movement_speed=2.000",
            "smooth_turn=true",
        ):
            self.assertIn(expected, text)

    def test_all_room_environment_choices_validate_and_export(self):
        for environment in qbx_config.ROOM_ENVIRONMENTS:
            with self.subTest(environment=environment):
                cfg = copy.deepcopy(qbx_config.DEFAULT_CONFIG)
                cfg["room"]["environment"] = environment
                qbx_config.validate_config(cfg)
                with tempfile.TemporaryDirectory() as tmp:
                    target = pathlib.Path(tmp) / "config.ini"
                    qbx_config.save_quest_ini(cfg, target)
                    self.assertIn(f"environment={environment}", target.read_text(encoding="utf-8"))

    def test_invalid_room_enum_is_rejected(self):
        cfg = copy.deepcopy(qbx_config.DEFAULT_CONFIG)
        cfg["room"]["environment"] = "random_world"
        with self.assertRaisesRegex(ValueError, "room.environment"):
            qbx_config.validate_config(cfg)

    def test_fake_vr_ini_is_explicitly_opt_in(self):
        cfg = copy.deepcopy(qbx_config.DEFAULT_CONFIG)
        with tempfile.TemporaryDirectory() as tmp:
            path = pathlib.Path(tmp) / "fake_vr.ini"
            qbx_config.save_fake_vr_ini(cfg, path)
            text = path.read_text(encoding="utf-8")
        self.assertIn("enabled=false", text)
        self.assertIn("allowed_processes=RobloxPlayerBeta.exe", text)
        self.assertIn("yaw_enabled=true", text)
        self.assertIn("max_delta_pixels=70", text)

    def test_fake_vr_sensitivity_range_is_validated(self):
        cfg = copy.deepcopy(qbx_config.DEFAULT_CONFIG)
        cfg["fake_vr"]["sensitivity_px_per_radian"] = 9000
        with self.assertRaisesRegex(ValueError, "fake_vr.sensitivity_px_per_radian"):
            qbx_config.validate_config(cfg)

    def test_profile_filename_is_safe_for_windows(self):
        self.assertEqual(qbx_config.profile_filename("My Nice: Profile"), "my_nice_profile.json")
        with self.assertRaisesRegex(ValueError, "letters or numbers"):
            qbx_config.profile_filename("!!!")


if __name__ == "__main__":
    unittest.main(verbosity=2)
