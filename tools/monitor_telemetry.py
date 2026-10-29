#!/usr/bin/env python3
"""Monitor the QuestBridge Windows host's localhost tracking/controller feed."""
from __future__ import annotations

import argparse
import ipaddress
import socket
import struct
import sys
import time
from pathlib import Path
from typing import Any

try:
    from qbx_config import DEFAULT_CONFIG_PATH, load_config
except ModuleNotFoundError:
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    from qbx_config import DEFAULT_CONFIG_PATH, load_config

MAGIC = 0x31584251
PROTOCOL_VERSION = 1
HEADER = struct.Struct("<IHHII")
TRACKING = struct.Struct("<Q3f4f3f3fI")
CONTROLLER = struct.Struct("<QBBHI2f2f3f4f")
TYPE_TRACKING = 3
TYPE_CONTROLLER = 4


def decode_packet(datagram: bytes) -> tuple[int, int, bytes]:
    if len(datagram) < HEADER.size:
        raise ValueError("short QBX header")
    magic, version, packet_type, sequence, payload_size = HEADER.unpack_from(datagram)
    if magic != MAGIC:
        raise ValueError("bad QBX magic")
    if version != PROTOCOL_VERSION:
        raise ValueError(f"unsupported protocol version {version}")
    if payload_size != len(datagram) - HEADER.size:
        raise ValueError("payload length mismatch")
    payload = datagram[HEADER.size:]
    if packet_type == TYPE_TRACKING and len(payload) != TRACKING.size:
        raise ValueError("invalid TRACKING payload length")
    if packet_type == TYPE_CONTROLLER and len(payload) != CONTROLLER.size:
        raise ValueError("invalid CONTROLLER payload length")
    return packet_type, sequence, payload


def decode_tracking(payload: bytes) -> dict[str, Any]:
    (timestamp, px, py, pz, qx, qy, qz, qw,
     vx, vy, vz, wx, wy, wz, flags) = TRACKING.unpack(payload)
    return {
        "timestamp_ns": timestamp,
        "position": (px, py, pz),
        "orientation": (qx, qy, qz, qw),
        "linear_velocity": (vx, vy, vz),
        "angular_velocity": (wx, wy, wz),
        "flags": flags,
    }


def decode_controller(payload: bytes) -> dict[str, Any]:
    (timestamp, hand, tracking_flags, _reserved, buttons, trigger, grip,
     stick_x, stick_y, px, py, pz, qx, qy, qz, qw) = CONTROLLER.unpack(payload)
    if hand not in (0, 1):
        raise ValueError(f"invalid controller hand {hand}")
    return {
        "timestamp_ns": timestamp,
        "hand": hand,
        "tracking_flags": tracking_flags,
        "buttons": buttons,
        "trigger": trigger,
        "grip": grip,
        "thumbstick": (stick_x, stick_y),
        "position": (px, py, pz),
        "orientation": (qx, qy, qz, qw),
    }


def fmt_vec(values: tuple[float, ...], precision: int = 3) -> str:
    return "(" + ", ".join(f"{value:.{precision}f}" for value in values) + ")"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", default=str(DEFAULT_CONFIG_PATH))
    parser.add_argument("--host", default="127.0.0.1", help="listener address (localhost recommended)")
    parser.add_argument("--port", type=int, help="telemetry UDP port; defaults to config telemetry_port")
    args = parser.parse_args()

    try:
        ipaddress.IPv4Address(args.host)
        cfg = load_config(args.config)
        port = args.port if args.port is not None else int(cfg["network"]["telemetry_port"])
        if not 1 <= port <= 65535:
            raise ValueError("monitor port must be in 1..65535; telemetry_port=0 disables the feed")
    except (OSError, ValueError) as exc:
        print(f"Telemetry monitor configuration error: {exc}", file=sys.stderr)
        return 2

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sock.bind((args.host, port))
    except OSError as exc:
        sock.close()
        print(f"Could not bind {args.host}:{port}: {exc}", file=sys.stderr)
        return 2
    sock.settimeout(0.5)
    print(f"Listening for QuestBridge telemetry on {args.host}:{port}. Ctrl+C to stop.", flush=True)
    latest_tracking: dict[str, Any] | None = None
    latest_controllers: dict[int, dict[str, Any]] = {}
    tracking_count = controller_count = invalid_count = 0
    last_report = time.monotonic()
    try:
        while True:
            try:
                data, _peer = sock.recvfrom(2048)
            except socket.timeout:
                data = b""
            except KeyboardInterrupt:
                break
            if data:
                try:
                    packet_type, _sequence, payload = decode_packet(data)
                    if packet_type == TYPE_TRACKING:
                        latest_tracking = decode_tracking(payload)
                        tracking_count += 1
                    elif packet_type == TYPE_CONTROLLER:
                        value = decode_controller(payload)
                        latest_controllers[value["hand"]] = value
                        controller_count += 1
                except ValueError:
                    invalid_count += 1
            now = time.monotonic()
            if now - last_report >= 1.0:
                print(f"~{tracking_count:4d} HMD / {controller_count:4d} controller packets/s; invalid={invalid_count}")
                if latest_tracking:
                    t = latest_tracking
                    print(f"  HMD pos={fmt_vec(t['position'])} quat={fmt_vec(t['orientation'])} flags=0x{t['flags']:X}")
                for hand in (0, 1):
                    c = latest_controllers.get(hand)
                    if c:
                        name = "Left" if hand == 0 else "Right"
                        print(f"  {name} pos={fmt_vec(c['position'])} stick={fmt_vec(c['thumbstick'])} "
                              f"trigger={c['trigger']:.2f} grip={c['grip']:.2f} buttons=0x{c['buttons']:X} "
                              f"flags=0x{c['tracking_flags']:X}")
                tracking_count = controller_count = invalid_count = 0
                last_report = now
    except KeyboardInterrupt:
        pass
    finally:
        sock.close()
    print("Telemetry monitor stopped.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
