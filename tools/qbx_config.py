#!/usr/bin/env python3
"""Configuration loading, validation, profiles, and Quest INI export for QuestBridge XR."""
from __future__ import annotations

from qbx_paths import project_root

import copy
import ipaddress
import json
import pathlib
from typing import Any

ROOT = project_root(__file__)
DEFAULT_CONFIG_PATH = ROOT / "config" / "questbridge.json"
PROFILE_DIR = ROOT / "config" / "profiles"

# Keep Studio, validation, and Quest INI export aligned with the native room renderer.
ROOM_ENVIRONMENTS = (
    "blossom",
    "night_city",
    "golden_city",
    "aurora",
    "cyberpunk",
    "sunset_beach",
    "snowy_mountains",
    "enchanted_forest",
    "desert_dunes",
    "ocean_cliffs",
    "space_nebula",
    "lunar_base",
    "volcanic",
    "underwater",
    "autumn_forest",
    "tropical_island",
    "candy_land",
    "steampunk_city",
    "rainy_tokyo",
    "sakura_night",
    "alpine_lake",
    "emerald_valley",
    "wasteland",
    "midnight_clouds",
    "crystal_cavern",
    "lavender_fields",
    "firefly_swamp",
    "polar_station",
)

DEFAULT_CONFIG: dict[str, Any] = {
    "schema_version": 1,
    "profile": "Balanced",
    "network": {
        "pc_ip": "192.168.1.100",
        "quest_ip": "192.168.1.42",
        "host_port": 9944,
        "quest_local_port": 9944,
        "telemetry_port": 9945,
    },
    "stream": {
        "source": "test",
        "width": 1920,
        "height": 1080,
        "fps": 72,
        "bitrate_bps": 8000000,
        "layout": "mono",
        "encoder": "libx264",
        "preset": "ultrafast",
        "tune": "zerolatency",
        "h264_profile": "high",
        "rate_control": "vbr",
        "maxrate_factor": 1.5,
        "vbv_buffer_ms": 100,
        "threads": 0,
        "udp_send_buffer_kib": 2048,
        "gop_seconds": 1.0,
        "draw_mouse": True,
        "config_repeat_seconds": 2.0,
    },
    "tracking": {
        "enabled": True,
        "controllers_enabled": True,
        "rate_hz": 72,
        "position_scale": 1.0,
        "thumbstick_deadzone": 0.12,
        "invert_thumbstick_x": False,
        "invert_thumbstick_y": False,
        "swap_face_buttons": False,
        "swap_hands": False,
        "controller_pose": "grip",
        "reference_space": "local",
        "trigger_deadzone": 0.02,
        "trigger_curve": 1.0,
        "offset_x": 0.0,
        "offset_y": 0.0,
        "offset_z": 0.0,
    },
    "display": {
        "brightness": 0.0,
        "contrast": 1.0,
        "saturation": 1.0,
        "gamma": 1.0,
        "aspect_mode": "stretch",
        "flip_horizontal": False,
        "flip_vertical": False,
        "swap_eyes": False,
    },
    "transport": {
        "max_frame_mib": 16,
        "incomplete_frame_timeout_ms": 120,
        "quest_receive_buffer_kib": 4096,
    },
    "room": {
        "environment": "blossom",
        "room_size": "medium",
        "screen_destination": "monitor",
        "monitor_count": 1,
        "show_tv": True,
        "show_couch": True,
        "show_chair": True,
        "room_controls_enabled": True,
        "sitting_toggle_enabled": True,
        "sitting_head_drop_m": 0.42,
        "movement_enabled": True,
        "movement_speed": 1.35,
        "smooth_turn": False,
        "turn_speed": 1.8,
        "snap_turn_degrees": 30.0,
        "comfort_vignette": True,
        "floor_offset_y": -1.6,
    },
    "fake_vr": {
        "enabled": False,
        "allowed_processes": "RobloxPlayerBeta.exe",
        "yaw_enabled": True,
        "pitch_enabled": True,
        "invert_pitch": False,
        "sensitivity_px_per_radian": 850.0,
        "pitch_sensitivity": 0.9,
        "deadzone_radians": 0.0015,
        "max_delta_pixels": 70,
    },
}


def deep_merge(base: dict[str, Any], overlay: dict[str, Any]) -> dict[str, Any]:
    result = copy.deepcopy(base)
    for key, value in overlay.items():
        if isinstance(value, dict) and isinstance(result.get(key), dict):
            result[key] = deep_merge(result[key], value)
        else:
            result[key] = copy.deepcopy(value)
    return result


def _number(value: Any, key: str, low: float, high: float) -> float:
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise ValueError(f"{key} must be a number")
    result = float(value)
    if not low <= result <= high:
        raise ValueError(f"{key} must be in the range {low:g}..{high:g}")
    return result


def _integer(value: Any, key: str, low: int, high: int) -> int:
    if isinstance(value, bool) or not isinstance(value, int):
        raise ValueError(f"{key} must be an integer")
    if not low <= value <= high:
        raise ValueError(f"{key} must be in the range {low}..{high}")
    return value


def validate_config(cfg: dict[str, Any]) -> dict[str, Any]:
    """Validate all settings. Returns cfg to allow inline validation use."""
    if not isinstance(cfg, dict):
        raise ValueError("Configuration root must be an object")
    if type(cfg.get("schema_version")) is not int or cfg.get("schema_version") != 1:
        raise ValueError("Unsupported config schema_version; expected integer 1")
    if not isinstance(cfg.get("profile"), str) or not cfg["profile"].strip():
        raise ValueError("profile must be a non-empty string")

    required_fields = {
        "network": ("pc_ip", "quest_ip", "host_port", "quest_local_port", "telemetry_port"),
        "stream": ("source", "width", "height", "fps", "bitrate_bps", "layout", "encoder", "preset", "tune", "h264_profile", "rate_control", "maxrate_factor", "vbv_buffer_ms", "threads", "udp_send_buffer_kib", "gop_seconds", "draw_mouse", "config_repeat_seconds"),
        "tracking": ("enabled", "controllers_enabled", "rate_hz", "position_scale", "thumbstick_deadzone", "invert_thumbstick_x", "invert_thumbstick_y", "swap_face_buttons", "swap_hands", "controller_pose", "reference_space", "trigger_deadzone", "trigger_curve", "offset_x", "offset_y", "offset_z"),
        "display": ("brightness", "contrast", "saturation", "gamma", "aspect_mode", "flip_horizontal", "flip_vertical", "swap_eyes"),
        "transport": ("max_frame_mib", "incomplete_frame_timeout_ms", "quest_receive_buffer_kib"),
        "room": ("environment", "room_size", "screen_destination", "monitor_count", "show_tv", "show_couch", "show_chair", "room_controls_enabled", "sitting_toggle_enabled", "sitting_head_drop_m", "movement_enabled", "movement_speed", "smooth_turn", "turn_speed", "snap_turn_degrees", "comfort_vignette", "floor_offset_y"),
        "fake_vr": ("enabled", "allowed_processes", "yaw_enabled", "pitch_enabled", "invert_pitch", "sensitivity_px_per_radian", "pitch_sensitivity", "deadzone_radians", "max_delta_pixels"),
    }
    for section, fields in required_fields.items():
        value = cfg.get(section)
        if not isinstance(value, dict):
            raise ValueError(f"{section} must be a JSON object")
        for key in fields:
            if key not in value:
                raise ValueError(f"{section}.{key} is required")

    net = cfg["network"]
    for key in ("pc_ip", "quest_ip"):
        try:
            ipaddress.IPv4Address(net[key])
        except (ipaddress.AddressValueError, TypeError, KeyError) as exc:
            raise ValueError(f"network.{key} must be an IPv4 address") from exc
    _integer(net["host_port"], "network.host_port", 1, 65535)
    _integer(net["quest_local_port"], "network.quest_local_port", 1, 65535)
    _integer(net["telemetry_port"], "network.telemetry_port", 0, 65535)
    if net["telemetry_port"] and net["telemetry_port"] == net["host_port"]:
        raise ValueError("network.telemetry_port must differ from network.host_port (or be 0 to disable)")

    stream = cfg["stream"]
    if stream["source"] not in ("test", "desktop", "mirror"):
        raise ValueError("stream.source must be 'test', 'desktop', or 'mirror'")
    if stream["layout"] not in ("mono", "sbs"):
        raise ValueError("stream.layout must be 'mono' or 'sbs'")
    if stream["encoder"] != "libx264":
        raise ValueError("stream.encoder must be 'libx264' (the packetizer currently supports H.264 Annex-B only)")
    if stream["preset"] not in ("ultrafast", "superfast", "veryfast", "faster", "fast", "medium"):
        raise ValueError("stream.preset must be a supported libx264 preset")
    if stream["tune"] not in ("zerolatency", "fastdecode", "film", "animation", "grain", "stillimage"):
        raise ValueError("stream.tune must be a supported libx264 tune")
    if stream["h264_profile"] not in ("baseline", "main", "high"):
        raise ValueError("stream.h264_profile must be baseline, main, or high")
    if stream["rate_control"] not in ("cbr", "vbr"):
        raise ValueError("stream.rate_control must be cbr or vbr")
    _number(stream["maxrate_factor"], "stream.maxrate_factor", 1.0, 3.0)
    _integer(stream["vbv_buffer_ms"], "stream.vbv_buffer_ms", 10, 2000)
    _integer(stream["threads"], "stream.threads", 0, 64)
    _integer(stream["udp_send_buffer_kib"], "stream.udp_send_buffer_kib", 64, 16384)
    _integer(stream["width"], "stream.width", 64, 4096)
    _integer(stream["height"], "stream.height", 64, 4096)
    if stream["layout"] == "sbs" and stream["width"] % 2:
        raise ValueError("stream.width must be even for side-by-side stereo")
    _integer(stream["fps"], "stream.fps", 1, 240)
    _integer(stream["bitrate_bps"], "stream.bitrate_bps", 250_000, 200_000_000)
    _number(stream["gop_seconds"], "stream.gop_seconds", 0.1, 10.0)
    _number(stream["config_repeat_seconds"], "stream.config_repeat_seconds", 0.25, 30.0)
    if not isinstance(stream["draw_mouse"], bool):
        raise ValueError("stream.draw_mouse must be true or false")

    tracking = cfg["tracking"]
    for key in ("enabled", "controllers_enabled", "invert_thumbstick_x", "invert_thumbstick_y", "swap_face_buttons", "swap_hands"):
        if not isinstance(tracking[key], bool):
            raise ValueError(f"tracking.{key} must be true or false")
    if tracking["controller_pose"] not in ("grip", "aim"):
        raise ValueError("tracking.controller_pose must be 'grip' or 'aim'")
    if tracking["reference_space"] not in ("local", "stage"):
        raise ValueError("tracking.reference_space must be 'local' or 'stage'")
    _integer(tracking["rate_hz"], "tracking.rate_hz", 10, 240)
    _number(tracking["position_scale"], "tracking.position_scale", 0.1, 5.0)
    _number(tracking["thumbstick_deadzone"], "tracking.thumbstick_deadzone", 0.0, 0.9)
    _number(tracking["trigger_deadzone"], "tracking.trigger_deadzone", 0.0, 0.5)
    _number(tracking["trigger_curve"], "tracking.trigger_curve", 0.25, 3.0)
    for key in ("offset_x", "offset_y", "offset_z"):
        _number(tracking[key], f"tracking.{key}", -2.0, 2.0)

    display = cfg["display"]
    _number(display["brightness"], "display.brightness", -0.5, 0.5)
    _number(display["contrast"], "display.contrast", 0.25, 2.5)
    _number(display["saturation"], "display.saturation", 0.0, 2.0)
    _number(display["gamma"], "display.gamma", 0.5, 2.5)
    if display["aspect_mode"] not in ("stretch", "fit", "crop"):
        raise ValueError("display.aspect_mode must be stretch, fit, or crop")
    for key in ("swap_eyes", "flip_horizontal", "flip_vertical"):
        if not isinstance(display[key], bool):
            raise ValueError(f"display.{key} must be true or false")

    transport = cfg["transport"]
    _integer(transport["max_frame_mib"], "transport.max_frame_mib", 1, 32)
    _integer(transport["incomplete_frame_timeout_ms"], "transport.incomplete_frame_timeout_ms", 20, 1000)
    _integer(transport["quest_receive_buffer_kib"], "transport.quest_receive_buffer_kib", 256, 16384)

    room = cfg["room"]
    if room["environment"] not in ROOM_ENVIRONMENTS:
        raise ValueError("room.environment must be one of: " + ", ".join(ROOM_ENVIRONMENTS))
    if room["room_size"] not in ("small", "medium", "large"):
        raise ValueError("room.room_size must be small, medium, or large")
    if room["screen_destination"] not in ("monitor", "tv", "both"):
        raise ValueError("room.screen_destination must be monitor, tv, or both")
    _integer(room["monitor_count"], "room.monitor_count", 1, 3)
    for key in ("show_tv", "show_couch", "show_chair", "room_controls_enabled", "sitting_toggle_enabled", "movement_enabled", "smooth_turn", "comfort_vignette"):
        if not isinstance(room[key], bool):
            raise ValueError(f"room.{key} must be true or false")
    _number(room["sitting_head_drop_m"], "room.sitting_head_drop_m", 0.1, 0.75)
    _number(room["movement_speed"], "room.movement_speed", 0.1, 4.0)
    _number(room["turn_speed"], "room.turn_speed", 0.2, 3.5)
    _number(room["snap_turn_degrees"], "room.snap_turn_degrees", 15.0, 60.0)
    _number(room["floor_offset_y"], "room.floor_offset_y", -2.5, 0.0)

    fake = cfg["fake_vr"]
    if not isinstance(fake["enabled"], bool) or not isinstance(fake["yaw_enabled"], bool) or not isinstance(fake["pitch_enabled"], bool) or not isinstance(fake["invert_pitch"], bool):
        raise ValueError("fake_vr enabled/yaw_enabled/pitch_enabled/invert_pitch must be true or false")
    if not isinstance(fake["allowed_processes"], str) or not fake["allowed_processes"].strip():
        raise ValueError("fake_vr.allowed_processes must be a non-empty comma-separated process allowlist")
    _number(fake["sensitivity_px_per_radian"], "fake_vr.sensitivity_px_per_radian", 50.0, 5000.0)
    _number(fake["pitch_sensitivity"], "fake_vr.pitch_sensitivity", 0.1, 3.0)
    _number(fake["deadzone_radians"], "fake_vr.deadzone_radians", 0.0, 0.1)
    _integer(fake["max_delta_pixels"], "fake_vr.max_delta_pixels", 1, 250)
    return cfg


def load_config(path: str | pathlib.Path | None = None) -> dict[str, Any]:
    source = pathlib.Path(path) if path else DEFAULT_CONFIG_PATH
    if not source.exists():
        cfg = copy.deepcopy(DEFAULT_CONFIG)
        validate_config(cfg)
        return cfg
    try:
        raw = json.loads(source.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        raise ValueError(f"Invalid JSON in {source}: {exc}") from exc
    if not isinstance(raw, dict):
        raise ValueError("Configuration root must be a JSON object")
    cfg = deep_merge(DEFAULT_CONFIG, raw)
    validate_config(cfg)
    return cfg


def save_config(cfg: dict[str, Any], path: str | pathlib.Path | None = None) -> pathlib.Path:
    validate_config(cfg)
    target = pathlib.Path(path) if path else DEFAULT_CONFIG_PATH
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(cfg, indent=2) + "\n", encoding="utf-8")
    return target


def profile_filename(name: str) -> str:
    """Make a cross-platform, path-safe profile filename stem."""
    cleaned = "".join(ch.lower() if ch.isalnum() else "_" for ch in name.strip()).strip("_")
    while "__" in cleaned:
        cleaned = cleaned.replace("__", "_")
    if not cleaned:
        raise ValueError("Profile name must contain letters or numbers")
    return cleaned + ".json"


def profile_names() -> list[str]:
    names = {"Balanced", "Low Latency", "High Quality", "Network Debug"}
    builtins = {name.casefold() for name in names}
    if PROFILE_DIR.exists():
        for path in PROFILE_DIR.glob("*.json"):
            display = path.stem.replace("_", " ")
            try:
                raw = json.loads(path.read_text(encoding="utf-8"))
                if isinstance(raw, dict) and isinstance(raw.get("profile"), str) and raw["profile"].strip():
                    display = raw["profile"].strip()
            except (OSError, json.JSONDecodeError):
                # Keep discoverability for a damaged file; selecting it will show the
                # more detailed parse error from apply_profile().
                pass
            if display.casefold() not in builtins:
                names.add(display)
    return sorted(names, key=str.casefold)


def apply_profile(name: str) -> dict[str, Any]:
    """Return defaults plus a profile overlay; users can freely edit after applying."""
    safe_name = name.strip()
    profile_path = PROFILE_DIR / profile_filename(safe_name)
    cfg = copy.deepcopy(DEFAULT_CONFIG)
    if profile_path.exists():
        try:
            overlay = json.loads(profile_path.read_text(encoding="utf-8"))
        except json.JSONDecodeError as exc:
            raise ValueError(f"Invalid profile JSON: {profile_path}: {exc}") from exc
        if not isinstance(overlay, dict):
            raise ValueError("Profile root must be a JSON object")
        cfg = deep_merge(cfg, overlay)
    else:
        # Built-in starter profiles provide different trade-offs without claiming to be
        # hardware-specific optimal values.
        profiles: dict[str, dict[str, Any]] = {
            "Balanced": {},
            "Low Latency": {
                "stream": {"width": 1280, "height": 720, "fps": 72, "bitrate_bps": 5_000_000,
                           "preset": "ultrafast", "gop_seconds": 0.5},
                "tracking": {"rate_hz": 90},
                "transport": {"incomplete_frame_timeout_ms": 80},
            },
            "High Quality": {
                "stream": {"width": 1920, "height": 1080, "fps": 90, "bitrate_bps": 18_000_000,
                           "preset": "veryfast", "gop_seconds": 1.0},
                "tracking": {"rate_hz": 90},
            },
            "Network Debug": {
                "stream": {"source": "test", "width": 1280, "height": 720, "fps": 30,
                           "bitrate_bps": 2_000_000, "preset": "ultrafast", "gop_seconds": 1.0},
                "tracking": {"rate_hz": 30},
            },
        }
        if safe_name not in profiles:
            raise ValueError(f"Unknown profile: {name}")
        cfg = deep_merge(cfg, profiles[safe_name])
    cfg["profile"] = safe_name
    validate_config(cfg)
    return cfg


def save_quest_ini(cfg: dict[str, Any], path: str | pathlib.Path) -> pathlib.Path:
    """Export the settings understood by the native Quest app as key=value INI."""
    validate_config(cfg)
    target = pathlib.Path(path)
    target.parent.mkdir(parents=True, exist_ok=True)
    tracking = cfg["tracking"]
    display = cfg["display"]
    transport = cfg["transport"]
    room = cfg["room"]
    text = (
        "# Generated by QuestBridge Studio. Edit in Studio to preserve validation.\n"
        f"host_ip={cfg['network']['pc_ip']}\n"
        f"host_port={cfg['network']['host_port']}\n"
        f"local_port={cfg['network']['quest_local_port']}\n"
        f"tracking_enabled={str(tracking['enabled']).lower()}\n"
        f"controllers_enabled={str(tracking['controllers_enabled']).lower()}\n"
        f"tracking_hz={tracking['rate_hz']}\n"
        f"position_scale={tracking['position_scale']:.3f}\n"
        f"thumbstick_deadzone={tracking['thumbstick_deadzone']:.3f}\n"
        f"invert_thumbstick_x={str(tracking['invert_thumbstick_x']).lower()}\n"
        f"invert_thumbstick_y={str(tracking['invert_thumbstick_y']).lower()}\n"
        f"swap_face_buttons={str(tracking['swap_face_buttons']).lower()}\n"
        f"swap_hands={str(tracking['swap_hands']).lower()}\n"
        f"controller_pose={tracking['controller_pose']}\n"
        f"reference_space={tracking['reference_space']}\n"
        f"trigger_deadzone={tracking['trigger_deadzone']:.3f}\n"
        f"trigger_curve={tracking['trigger_curve']:.3f}\n"
        f"tracking_offset_x={tracking['offset_x']:.3f}\n"
        f"tracking_offset_y={tracking['offset_y']:.3f}\n"
        f"tracking_offset_z={tracking['offset_z']:.3f}\n"
        f"brightness={display['brightness']:.3f}\n"
        f"contrast={display['contrast']:.3f}\n"
        f"saturation={display['saturation']:.3f}\n"
        f"gamma={display['gamma']:.3f}\n"
        f"aspect_mode={display['aspect_mode']}\n"
        f"flip_horizontal={str(display['flip_horizontal']).lower()}\n"
        f"flip_vertical={str(display['flip_vertical']).lower()}\n"
        f"swap_eyes={str(display['swap_eyes']).lower()}\n"
        f"max_frame_mib={transport['max_frame_mib']}\n"
        f"incomplete_frame_timeout_ms={transport['incomplete_frame_timeout_ms']}\n"
        f"quest_receive_buffer_kib={transport['quest_receive_buffer_kib']}\n"
        f"environment={room['environment']}\n"
        f"room_size={room['room_size']}\n"
        f"screen_destination={room['screen_destination']}\n"
        f"monitor_count={room['monitor_count']}\n"
        f"show_tv={str(room['show_tv']).lower()}\n"
        f"show_couch={str(room['show_couch']).lower()}\n"
        f"show_chair={str(room['show_chair']).lower()}\n"
        f"room_controls_enabled={str(room['room_controls_enabled']).lower()}\n"
        f"sitting_toggle_enabled={str(room['sitting_toggle_enabled']).lower()}\n"
        f"sitting_head_drop_m={room['sitting_head_drop_m']:.3f}\n"
        f"room_movement_enabled={str(room['movement_enabled']).lower()}\n"
        f"movement_speed={room['movement_speed']:.3f}\n"
        f"smooth_turn={str(room['smooth_turn']).lower()}\n"
        f"turn_speed={room['turn_speed']:.3f}\n"
        f"snap_turn_degrees={room['snap_turn_degrees']:.1f}\n"
        f"comfort_vignette={str(room['comfort_vignette']).lower()}\n"
        f"room_floor_offset_y={room['floor_offset_y']:.3f}\n"
    )
    target.write_text(text, encoding="utf-8")
    return target


def save_fake_vr_ini(cfg: dict[str, Any], path: str | pathlib.Path) -> pathlib.Path:
    """Export safe opt-in desktop mouse-look settings consumed by QuestBridgeHost."""
    validate_config(cfg)
    fake = cfg["fake_vr"]
    target = pathlib.Path(path)
    target.parent.mkdir(parents=True, exist_ok=True)
    text = (
        "# Generated by QuestBridge Studio. The bridge is opt-in and only sends input while the allowlisted process is foreground.\n"
        f"enabled={str(fake['enabled']).lower()}\n"
        f"allowed_processes={fake['allowed_processes']}\n"
        f"yaw_enabled={str(fake['yaw_enabled']).lower()}\n"
        f"pitch_enabled={str(fake['pitch_enabled']).lower()}\n"
        f"invert_pitch={str(fake['invert_pitch']).lower()}\n"
        f"sensitivity_px_per_radian={fake['sensitivity_px_per_radian']:.2f}\n"
        f"pitch_sensitivity={fake['pitch_sensitivity']:.3f}\n"
        f"deadzone_radians={fake['deadzone_radians']:.4f}\n"
        f"max_delta_pixels={fake['max_delta_pixels']}\n"
    )
    target.write_text(text, encoding="utf-8")
    return target
