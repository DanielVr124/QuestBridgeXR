#!/usr/bin/env python3
"""QuestBridge XR environment checks and optional UDP host handshake probe.

This tool performs read-only local checks. --probe-host sends a QBX HELLO datagram
and waits for HELLO_ACK; it does not change Windows OpenXR settings or the registry.
"""
from __future__ import annotations

from qbx_paths import project_root

import argparse
import os
import pathlib
import shutil
import socket
import struct
import subprocess
import sys
from dataclasses import dataclass
from typing import Iterable

try:
    from qbx_config import apply_profile, load_config, profile_names, validate_config
except ModuleNotFoundError:  # Support import from repository root too.
    sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
    from qbx_config import apply_profile, load_config, profile_names, validate_config

ROOT = project_root(__file__)
MAGIC = 0x31584251
PROTOCOL_VERSION = 1
TYPE_HELLO = 1
TYPE_HELLO_ACK = 2
HEADER = struct.Struct("<IHHII")
HELLO = struct.Struct("<II32s")
HELLO_ACK = struct.Struct("<IIHH")


@dataclass
class Check:
    level: str
    name: str
    detail: str


def make_hello(sequence: int = 1, device_name: str = "QuestBridge Doctor") -> bytes:
    name = device_name.encode("ascii", errors="replace")[:31].ljust(32, b"\0")
    payload = HELLO.pack(1, 0x7, name)
    return HEADER.pack(MAGIC, PROTOCOL_VERSION, TYPE_HELLO, sequence & 0xFFFFFFFF, len(payload)) + payload


def parse_hello_ack(data: bytes, expected_sequence: int = 1) -> tuple[int, int]:
    if len(data) < HEADER.size:
        raise ValueError("reply is shorter than the QBX packet header")
    magic, version, packet_type, sequence, payload_bytes = HEADER.unpack_from(data)
    if magic != MAGIC:
        raise ValueError("reply has the wrong QBX magic")
    if version != PROTOCOL_VERSION:
        raise ValueError(f"protocol version mismatch ({version})")
    if packet_type != TYPE_HELLO_ACK:
        raise ValueError(f"expected HELLO_ACK ({TYPE_HELLO_ACK}), received type {packet_type}")
    if sequence != (expected_sequence & 0xFFFFFFFF):
        raise ValueError(f"reply sequence mismatch ({sequence})")
    if payload_bytes != HELLO_ACK.size or len(data) != HEADER.size + HELLO_ACK.size:
        raise ValueError("HELLO_ACK payload has the wrong size")
    server_version, capabilities, _stream_port, _reserved = HELLO_ACK.unpack_from(data, HEADER.size)
    if server_version == 0:
        raise ValueError("server reports an invalid protocol version")
    return server_version, capabilities


def probe_host(config: dict, timeout_seconds: float = 1.2) -> str:
    host = config["network"]["pc_ip"]
    port = config["network"]["host_port"]
    sequence = 0x51425831
    request = make_hello(sequence, "QuestBridge Doctor")
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.settimeout(timeout_seconds)
        sock.sendto(request, (host, port))
        try:
            data, peer = sock.recvfrom(4096)
        except socket.timeout as exc:
            raise TimeoutError(f"No HELLO_ACK from {host}:{port} within {timeout_seconds:.1f}s") from exc
    if peer[0] != host:
        raise ValueError(f"HELLO_ACK came from {peer[0]} instead of configured host {host}")
    server_version, caps = parse_hello_ack(data, sequence)
    return f"HELLO_ACK received from {peer[0]}:{peer[1]} (server protocol {server_version}, capabilities 0x{caps:x})."


def _check_file(checks: list[Check], relative: str, label: str) -> None:
    path = ROOT / relative
    checks.append(Check("PASS" if path.is_file() else "FAIL", label,
                        str(path.relative_to(ROOT)) if path.is_file() else f"Missing {path.relative_to(ROOT)}"))


def _command_version(command: str, args: Iterable[str] = ("--version",), timeout: float = 4.0) -> str | None:
    exe = shutil.which(command)
    if not exe:
        return None
    try:
        result = subprocess.run([exe, *args], capture_output=True, text=True, timeout=timeout, check=False)
    except (OSError, subprocess.TimeoutExpired):
        return f"{exe} (could not run version check)"
    summary = next((line.strip() for line in (result.stdout + "\n" + result.stderr).splitlines() if line.strip()), exe)
    return summary[:180]


def run_checks(config_path: str | pathlib.Path, include_host_probe: bool = False,
               timeout_seconds: float = 1.2) -> list[Check]:
    checks: list[Check] = []
    try:
        config = load_config(config_path)
        validate_config(config)
        checks.append(Check("PASS", "Active configuration", f"Validated {pathlib.Path(config_path).resolve()}"))
    except (OSError, ValueError) as exc:
        checks.append(Check("FAIL", "Active configuration", str(exc)))
        config = None

    try:
        names = profile_names()
        for name in names:
            validate_config(apply_profile(name))
        checks.append(Check("PASS", "Saved profiles", f"{len(names)} profiles validate: {', '.join(names)}"))
    except (OSError, ValueError) as exc:
        checks.append(Check("FAIL", "Saved profiles", str(exc)))

    expected_sizes = {
        "packet header": HEADER.size == 16,
        "HELLO payload": HELLO.size == 40,
        "HELLO_ACK payload": HELLO_ACK.size == 12,
    }
    all_sizes_ok = all(expected_sizes.values())
    checks.append(Check("PASS" if all_sizes_ok else "FAIL", "QBX wire-format sizes",
                        ", ".join(f"{name}={'OK' if valid else 'MISMATCH'}" for name, valid in expected_sizes.items())))

    for path, label in (
        ("host/src/main.cpp", "Windows host source"),
        ("host/CMakeLists.txt", "Windows host CMake target"),
        ("protocol/include/qbx/protocol.hpp", "Shared C++ protocol header"),
        ("quest-client/app/src/main/cpp/main.cpp", "Quest OpenXR client source"),
        ("quest-client/app/src/main/cpp/video_decoder.cpp", "Quest decoder source"),
        ("quest-client/app/src/main/AndroidManifest.xml", "Quest Android manifest"),
        ("scripts/build-windows.ps1", "Windows build script"),
        ("tools/stream_test_video.py", "FFmpeg video sender"),
        ("tools/openxr_runtime.py", "Per-process OpenXR runtime helper"),
    ):
        _check_file(checks, path, label)

    ffmpeg = shutil.which("ffmpeg")
    if not ffmpeg:
        checks.append(Check("WARN", "FFmpeg", "Not found on PATH. Install a build containing libx264; desktop preview also needs gdigrab."))
    else:
        try:
            result = subprocess.run([ffmpeg, "-hide_banner", "-encoders"], capture_output=True,
                                    text=True, timeout=8, check=False)
            has_x264 = "libx264" in result.stdout + result.stderr
            checks.append(Check("PASS" if has_x264 else "WARN", "FFmpeg libx264 encoder",
                                "FFmpeg and libx264 are available." if has_x264 else
                                f"FFmpeg was found at {ffmpeg}, but libx264 was not listed."))
            if os.name == "nt":
                has_gdigrab = "gdigrab" in (result.stdout + result.stderr)
                checks.append(Check("PASS" if has_gdigrab else "WARN", "FFmpeg desktop capture",
                                    "gdigrab is listed." if has_gdigrab else "FFmpeg does not list gdigrab; desktop preview will not work."))
        except (OSError, subprocess.TimeoutExpired) as exc:
            checks.append(Check("WARN", "FFmpeg encoder probe", str(exc)))

    for command, label in (("cmake", "CMake"), ("java", "Java/JDK"), ("gradle", "Gradle"), ("adb", "Android Debug Bridge")):
        version = _command_version(command)
        checks.append(Check("INFO" if version else "WARN", label,
                            version if version else f"{command} not found on PATH; needed for some build/install tasks."))
    if os.name == "nt":
        compiler = shutil.which("cl") or shutil.which("msbuild")
        checks.append(Check("INFO" if compiler else "WARN", "Visual Studio C++ tools",
                            f"Found {compiler}" if compiler else
                            "cl.exe/msbuild.exe not found on PATH. Use a VS 2022 Developer PowerShell with Desktop development with C++."))
    else:
        checks.append(Check("INFO", "Windows C++ build", "This host target is Windows-only and cannot be compiled on this operating system."))

    host_candidates = [ROOT / "host/QuestBridgeHost.exe", ROOT / "build/host/Release/QuestBridgeHost.exe", ROOT / "build/host/Debug/QuestBridgeHost.exe"]
    host_exe = next((p for p in host_candidates if p.is_file()), None)
    checks.append(Check("PASS" if host_exe else "INFO", "Built Windows host executable",
                        str(host_exe.relative_to(ROOT)) if host_exe else "Not built yet; run scripts/build-windows.ps1 on Windows."))
    apk_candidates = [ROOT / "QuestClient/QuestBridgeXR-Quest-debug.apk",
                      ROOT / "quest-client/app/build/outputs/apk/debug/app-debug.apk"]
    apk = next((candidate for candidate in apk_candidates if candidate.is_file()), None)
    checks.append(Check("PASS" if apk else "INFO", "Quest debug APK",
                        str(apk.relative_to(ROOT)) if apk else "Not built yet; build from Android Studio/Gradle or download the CI APK artifact."))

    if include_host_probe:
        if config is None:
            checks.append(Check("FAIL", "PC host handshake", "Skipped because the active configuration is invalid."))
        else:
            try:
                detail = probe_host(config, timeout_seconds)
                checks.append(Check("PASS", "PC host handshake", detail))
            except (OSError, ValueError, TimeoutError) as exc:
                checks.append(Check("WARN", "PC host handshake", f"{exc}. Check the configured PC IP/UDP port and that QuestBridgeHost is running."))

    return checks


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", default=str(ROOT / "config" / "questbridge.json"), help="settings JSON")
    parser.add_argument("--probe-host", action="store_true", help="send a real QBX HELLO and wait for HELLO_ACK")
    parser.add_argument("--timeout", type=float, default=1.2, help="handshake timeout in seconds (0.1–10)")
    args = parser.parse_args()
    if not 0.1 <= args.timeout <= 10:
        parser.error("--timeout must be in 0.1..10 seconds")

    checks = run_checks(args.config, args.probe_host, args.timeout)
    print("QuestBridge XR diagnostics")
    print("=" * 28)
    for check in checks:
        print(f"[{check.level:4}] {check.name}: {check.detail}")
    fails = sum(item.level == "FAIL" for item in checks)
    warns = sum(item.level == "WARN" for item in checks)
    print("-" * 28)
    print(f"Summary: {len(checks)} checks; {fails} failures; {warns} warnings.")
    print("This checks setup and host handshake only. QuestBridge now includes an experimental D3D11 OpenXR runtime source/build target, but the Windows DLL and physical Quest rendering are not validated by this diagnostic.")
    return 1 if fails or (args.probe_host and any(c.name == "PC host handshake" and c.level != "PASS" for c in checks)) else 0


if __name__ == "__main__":
    raise SystemExit(main())
