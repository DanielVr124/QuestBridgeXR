#!/usr/bin/env python3
"""Read-only LAN address discovery helpers used by QuestBridge Studio."""
from __future__ import annotations

import ipaddress
import re
import shutil
import socket
import subprocess


_IPV4_TOKEN = re.compile(r"\b(?:inet|src)\s+(\d{1,3}(?:\.\d{1,3}){3})(?:/\d+)?\b")


def _usable_lan_ipv4(candidate: str) -> bool:
    try:
        address = ipaddress.IPv4Address(candidate)
    except ipaddress.AddressValueError:
        return False
    return not (
        address.is_loopback
        or address.is_link_local
        or address.is_unspecified
        or address.is_multicast
        or address.is_reserved
    )


def parse_lan_ipv4(output: str) -> str | None:
    """Extract the first usable LAN IPv4 address from common Android/Linux output."""
    for candidate in _IPV4_TOKEN.findall(output):
        if _usable_lan_ipv4(candidate):
            return candidate
    return None


def discover_pc_ipv4() -> str | None:
    """Find the local IPv4 chosen by the OS default route without sending a packet."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        # UDP connect selects a route/local address; no datagram is sent.
        sock.connect(("192.0.2.1", 9))
        candidate = sock.getsockname()[0]
        if _usable_lan_ipv4(candidate):
            return candidate
    except OSError:
        pass
    finally:
        sock.close()

    # Fallback for machines without a default route (or where the route probe is blocked).
    try:
        results = socket.getaddrinfo(socket.gethostname(), None, socket.AF_INET, socket.SOCK_DGRAM)
    except OSError:
        return None
    candidates = []
    for result in results:
        candidate = result[4][0]
        if _usable_lan_ipv4(candidate) and candidate not in candidates:
            candidates.append(candidate)
    return next((ip for ip in candidates if ipaddress.IPv4Address(ip).is_private),
                candidates[0] if candidates else None)


def discover_quest_ipv4() -> tuple[str | None, str]:
    """Read the Wi-Fi IPv4 from one authorized ADB device; never selects among multiple devices."""
    adb = shutil.which("adb")
    if not adb:
        return None, "ADB was not found. Install Android platform-tools and put adb on PATH."

    try:
        devices_result = subprocess.run(
            [adb, "devices"], capture_output=True, text=True, timeout=8, check=False
        )
    except (OSError, subprocess.SubprocessError) as exc:
        return None, f"Could not query ADB devices: {exc}"
    if devices_result.returncode != 0:
        detail = devices_result.stderr.strip() or devices_result.stdout.strip()
        return None, f"ADB device query failed: {detail or 'unknown error'}"

    online = []
    states = []
    for line in devices_result.stdout.splitlines()[1:]:
        fields = line.split()
        if len(fields) >= 2:
            states.append(f"{fields[0]} ({fields[1]})")
            if fields[1] == "device":
                online.append(fields[0])
    if len(online) != 1:
        if not online:
            state_detail = "; detected: " + ", ".join(states) if states else ""
            return None, "No authorized ADB device was found" + state_detail + ". Connect the Quest by USB and allow debugging."
        return None, f"Found {len(online)} authorized ADB devices. Leave only the intended Quest connected to avoid choosing the wrong device."

    serial = online[0]
    commands = (
        [adb, "-s", serial, "shell", "ip", "-4", "addr", "show", "wlan0"],
        [adb, "-s", serial, "shell", "ip", "addr", "show", "wlan0"],
        [adb, "-s", serial, "shell", "ip", "route", "get", "1.1.1.1"],
    )
    last_detail = ""
    for command in commands:
        try:
            result = subprocess.run(command, capture_output=True, text=True, timeout=8, check=False)
        except (OSError, subprocess.SubprocessError) as exc:
            last_detail = str(exc)
            continue
        last_detail = result.stderr.strip()
        if result.returncode == 0:
            address = parse_lan_ipv4(result.stdout)
            if address:
                return address, f"Quest Wi-Fi IPv4 discovered from ADB device {serial}."
    suffix = f" Details: {last_detail}" if last_detail else ""
    return None, "ADB found the Quest, but its Wi-Fi IPv4 could not be read. Confirm it is connected to Wi-Fi." + suffix
