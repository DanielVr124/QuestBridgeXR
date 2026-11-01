#!/usr/bin/env python3
"""Quest USB/ADB detection and live transport diagnostics for QuestBridge Studio.

ADB manages wired sessions; the Quest app and Windows sender carry stream packets
through a bidirectional TCP connection created by ADB reverse.
"""
from __future__ import annotations

import os
import pathlib
import shutil
import subprocess
from dataclasses import dataclass
from typing import Iterable


PACKAGE_NAME = "com.questbridge.xr"


@dataclass(frozen=True)
class AdbDevice:
    serial: str
    state: str
    details: str
    transport: str


def _adb_filename() -> str:
    return "adb.exe" if os.name == "nt" else "adb"


def find_adb_executable(project_root: str | pathlib.Path | None = None) -> str | None:
    """Find ADB on PATH, in common Android SDK locations, or in the portable bundle."""
    on_path = shutil.which("adb")
    if on_path:
        return str(pathlib.Path(on_path).resolve())

    filename = _adb_filename()
    candidates: list[pathlib.Path] = []
    for env_name in ("ANDROID_SDK_ROOT", "ANDROID_HOME"):
        value = os.environ.get(env_name)
        if value:
            candidates.append(pathlib.Path(value) / "platform-tools" / filename)

    local = os.environ.get("LOCALAPPDATA")
    if local:
        candidates.append(pathlib.Path(local) / "Android" / "Sdk" / "platform-tools" / filename)
    user = os.environ.get("USERPROFILE") or os.environ.get("HOME")
    if user:
        candidates.append(pathlib.Path(user) / "AppData" / "Local" / "Android" / "Sdk" / "platform-tools" / filename)

    if project_root is not None:
        root = pathlib.Path(project_root)
        candidates.extend((
            root / "platform-tools" / filename,
            root / "tools" / "platform-tools" / filename,
            root / filename,
        ))

    for candidate in candidates:
        if candidate.is_file():
            return str(candidate.resolve())
    return None


def parse_adb_devices(output: str) -> list[AdbDevice]:
    """Parse 'adb devices -l' and distinguish USB serials from tcpip/emulator entries."""
    devices: list[AdbDevice] = []
    for raw_line in output.splitlines():
        line = raw_line.strip()
        if not line or line.startswith("List of devices attached") or line.startswith("*"):
            continue
        fields = line.split()
        if len(fields) < 2:
            continue
        serial, state = fields[0], fields[1]
        details = " ".join(fields[2:])
        if serial.startswith("emulator-"):
            transport = "emulator"
        elif ":" in serial:
            transport = "network"
        else:
            # ADB's standard USB serials do not contain a host:port separator.
            transport = "usb"
        devices.append(AdbDevice(serial, state, details, transport))
    return devices


def _check(level: str, label: str, detail: str) -> dict[str, str]:
    return {"level": level, "label": label, "detail": detail}


def probe_usb_connection(
    adb_path: str | pathlib.Path | None = None,
    project_root: str | pathlib.Path | None = None,
    timeout_seconds: float = 8.0,
) -> dict:
    """Check the complete USB/ADB setup path without changing headset settings."""
    executable = str(adb_path) if adb_path else find_adb_executable(project_root)
    report = {
        "ok": False,
        "summary": "",
        "adb_path": executable,
        "serial": None,
        "transport": None,
        "model": None,
        "android_version": None,
        "app_installed": None,
        "app_running": None,
        "host_handshake_seen": None,
        "video_config_seen": None,
        "checks": [],
    }
    checks: list[dict[str, str]] = report["checks"]

    if not executable or not pathlib.Path(executable).is_file():
        report["adb_path"] = None
        checks.append(_check("FAIL", "ADB tools", "adb.exe was not found. Install Android Platform-Tools or use Locate ADB in Studio."))
        report["summary"] = "ADB is missing — install Android Platform-Tools or locate adb.exe."
        return report

    checks.append(_check("PASS", "ADB tools", f"Using {executable}"))
    try:
        devices_result = subprocess.run(
            [executable, "devices", "-l"],
            capture_output=True, text=True, timeout=timeout_seconds, check=False,
        )
    except (OSError, subprocess.SubprocessError) as exc:
        checks.append(_check("FAIL", "ADB server", f"Could not query ADB: {exc}"))
        report["summary"] = "ADB could not query connected devices."
        return report

    if devices_result.returncode != 0:
        detail = devices_result.stderr.strip() or devices_result.stdout.strip() or "ADB returned a failure code."
        checks.append(_check("FAIL", "ADB server", detail))
        report["summary"] = "ADB device query failed."
        return report

    checks.append(_check("PASS", "ADB server", "ADB responded and listed attached devices."))
    devices = parse_adb_devices(devices_result.stdout)
    usb_devices = [device for device in devices if device.transport == "usb"]
    if not usb_devices:
        network_devices = [device for device in devices if device.transport == "network" and device.state == "device"]
        if network_devices:
            names = ", ".join(device.serial for device in network_devices)
            checks.append(_check("WARN", "USB headset", f"No USB device found. ADB only sees network device(s): {names}. Connect a data-capable USB cable."))
            report["summary"] = "ADB is working, but no headset is connected over USB."
        else:
            states = ", ".join(f"{d.serial} ({d.state})" for d in devices)
            suffix = f" Other ADB entries: {states}." if states else ""
            checks.append(_check("WARN", "USB headset", "No Quest detected over USB. Check the cable is data-capable, unlock the headset, and enable USB debugging." + suffix))
            report["summary"] = "No USB headset detected."
        return report

    if len(usb_devices) != 1:
        names = ", ".join(f"{d.serial} ({d.state})" for d in usb_devices)
        checks.append(_check("WARN", "USB headset", f"Found {len(usb_devices)} USB Android devices: {names}. Disconnect other USB Android devices so Studio cannot choose the wrong headset."))
        report["summary"] = f"Found {len(usb_devices)} USB devices; leave only the intended Quest connected."
        return report

    device = usb_devices[0]
    report["serial"] = device.serial
    report["transport"] = device.transport
    if device.state == "unauthorized":
        checks.append(_check("WARN", "USB authorization", "The headset is visible but has not approved this PC. Put on the Quest and accept the 'Allow USB debugging' prompt."))
        report["summary"] = "Quest detected — waiting for USB debugging authorization on the headset."
        return report
    if device.state != "device":
        checks.append(_check("FAIL", "USB authorization", f"ADB reports device state '{device.state}'. Reconnect the cable and check the headset's USB debugging prompt."))
        report["summary"] = f"Quest is visible but ADB state is {device.state}."
        return report

    checks.append(_check("PASS", "USB authorization", "Quest accepted this PC's ADB key."))
    try:
        model_result = subprocess.run(
            [executable, "-s", device.serial, "shell", "getprop", "ro.product.model"],
            capture_output=True, text=True, timeout=timeout_seconds, check=False,
        )
        version_result = subprocess.run(
            [executable, "-s", device.serial, "shell", "getprop", "ro.build.version.release"],
            capture_output=True, text=True, timeout=timeout_seconds, check=False,
        )
    except (OSError, subprocess.SubprocessError) as exc:
        checks.append(_check("FAIL", "Quest response", f"ADB found the USB device, but shell access failed: {exc}"))
        report["summary"] = "USB is detected, but the Quest did not answer ADB shell commands."
        return report

    if model_result.returncode != 0:
        detail = model_result.stderr.strip() or "ADB shell did not return the Quest model."
        checks.append(_check("FAIL", "Quest response", detail))
        report["summary"] = "USB is detected, but the Quest did not answer ADB shell commands."
        return report

    model = model_result.stdout.strip() or "Android device"
    android_version = version_result.stdout.strip() if version_result.returncode == 0 else ""
    report["model"] = model
    report["android_version"] = android_version or None
    version_text = f", Android {android_version}" if android_version else ""
    if not any(token in model.casefold() for token in ("quest", "oculus", "meta")):
        checks.append(_check(
            "WARN", "Headset identification",
            f"USB device answered ADB, but its model is '{model}', not a recognized Meta Quest model. Check that the Quest is the device connected."
        ))
        report["summary"] = f"USB device responds, but it is not identified as a Meta Quest ({model})."
        return report
    checks.append(_check("PASS", "Quest response", f"{model}{version_text} answered an ADB shell request."))

    try:
        installed = subprocess.run(
            [executable, "-s", device.serial, "shell", "pm", "path", PACKAGE_NAME],
            capture_output=True, text=True, timeout=timeout_seconds, check=False,
        )
        is_installed = installed.returncode == 0 and "package:" in installed.stdout
        report["app_installed"] = is_installed
        checks.append(_check(
            "PASS" if is_installed else "INFO",
            "QuestBridge app",
            "QuestBridge XR is installed." if is_installed else "QuestBridge XR is not installed yet. Use Install / Launch below after the APK is available.",
        ))
    except (OSError, subprocess.SubprocessError) as exc:
        checks.append(_check("WARN", "QuestBridge app", f"Could not check package status: {exc}"))

    if report["app_installed"]:
        try:
            running = subprocess.run(
                [executable, "-s", device.serial, "shell", "pidof", PACKAGE_NAME],
                capture_output=True, text=True, timeout=timeout_seconds, check=False,
            )
            is_running = running.returncode == 0 and bool(running.stdout.strip())
            if not is_running:
                # Some Android builds omit pidof; use the process list as a fallback.
                process_list = subprocess.run(
                    [executable, "-s", device.serial, "shell", "ps", "-A"],
                    capture_output=True, text=True, timeout=timeout_seconds, check=False,
                )
                is_running = process_list.returncode == 0 and PACKAGE_NAME in process_list.stdout
            report["app_running"] = is_running
            checks.append(_check(
                "PASS" if is_running else "INFO",
                "QuestBridge process",
                "QuestBridge XR is currently running." if is_running else "The app is installed but not running. Launch it from the Quest library or use Install / Launch below.",
            ))
        except (OSError, subprocess.SubprocessError) as exc:
            checks.append(_check("WARN", "QuestBridge process", f"Could not check whether the app is running: {exc}"))

        # These logs distinguish "ADB can see the headset" from the Quest app
        # actually completing its UDP handshake and receiving stream metadata.
        try:
            log_result = subprocess.run(
                [executable, "-s", device.serial, "logcat", "-d", "-t", "1200", "-s", "QuestBridgeXR:I"],
                capture_output=True, text=True, timeout=max(timeout_seconds, 10.0), check=False,
            )
            if log_result.returncode == 0:
                quest_log = log_result.stdout.casefold()
                host_seen = ("windows host accepted connection" in quest_log
                             or "wired usb stream handshake accepted" in quest_log)
                video_seen = (
                    "received video config" in quest_log
                    or "video decoder configured:" in quest_log
                )
                report["host_handshake_seen"] = host_seen
                report["video_config_seen"] = video_seen
                checks.append(_check(
                    "PASS" if host_seen else "WARN",
                    "Quest → PC UDP handshake",
                    "The Quest log contains a Windows-host HELLO_ACK acceptance. This proves the Quest reached the configured PC at least once in the retained log; refresh after launching the app to check the current session."
                    if host_seen else
                    "No Windows-host acceptance was found in the latest retained QuestBridge logs. Start the Windows Host, launch QuestBridge on the headset, and check the PC IP, UDP port, and Windows Firewall."
                ))
                checks.append(_check(
                    "PASS" if video_seen else "INFO",
                    "Video stream setup",
                    "The Quest log shows video configuration/decoder setup was received. This confirms stream setup reached the headset, not that every frame is rendering smoothly."
                    if video_seen else
                    "No video configuration was found in the latest retained logs. Start the video sender after the Quest-to-PC handshake."
                ))
            else:
                detail = log_result.stderr.strip() or "Android logcat returned a failure code."
                checks.append(_check("WARN", "Quest connection logs", detail))
        except (OSError, subprocess.SubprocessError) as exc:
            checks.append(_check("WARN", "Quest connection logs", f"Could not read Quest logcat: {exc}"))

    report["ok"] = True
    report["summary"] = f"USB ready — {model} is responding to ADB."
    return report
