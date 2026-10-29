#!/usr/bin/env python3
"""Protocol connectivity simulator; sends fake pose/buttons, not real Quest tracking."""
import argparse
import math
import socket
import struct
import time
import random

MAGIC = 0x31584251
VERSION = 1
HELLO, HELLO_ACK, TRACKING, CONTROLLER, HEARTBEAT = 1, 2, 3, 4, 5
HEADER = struct.Struct('<IHHII')
HELLO_PAYLOAD = struct.Struct('<II32s')
HELLO_ACK_PAYLOAD = struct.Struct('<IIHH')
TRACKING_PAYLOAD = struct.Struct('<Q3f4f3f3fI')
CONTROLLER_PAYLOAD = struct.Struct('<QBBHIff2f3f4f')


def packet(kind, seq, payload=b''):
    return HEADER.pack(MAGIC, VERSION, kind, seq, len(payload)) + payload


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--host', default='127.0.0.1')
    ap.add_argument('--port', type=int, default=9944)
    ap.add_argument('--rate', type=float, default=30.0, help='synthetic tracking updates per second')
    args = ap.parse_args()
    if args.rate <= 0 or args.rate > 240:
        ap.error('--rate must be > 0 and <= 240')

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(2.0)
    address = (args.host, args.port)
    seq = 1
    hello = HELLO_PAYLOAD.pack(1, 0x3, b'QuestBridge Python Simulator')
    sock.sendto(packet(HELLO, seq, hello), address)
    try:
        data, peer = sock.recvfrom(2048)
        if len(data) < HEADER.size:
            raise RuntimeError('short reply')
        magic, version, kind, reply_seq, size = HEADER.unpack_from(data)
        if magic != MAGIC or version != VERSION or kind != HELLO_ACK or size != HELLO_ACK_PAYLOAD.size:
            raise RuntimeError(f'unexpected handshake reply from {peer}')
        server_version, caps, stream_port, _ = HELLO_ACK_PAYLOAD.unpack_from(data, HEADER.size)
        print(f'Connected to {peer}; server_version={server_version}, caps=0x{caps:x}, stream_port={stream_port}')
    except socket.timeout:
        raise SystemExit('No HELLO_ACK received. Is QuestBridgeHost running and is the UDP port allowed?')

    started = time.monotonic()
    next_tick = started
    interval = 1.0 / args.rate
    print('Sending synthetic head pose and both controller states. Ctrl+C to stop.')
    try:
        while True:
            now = time.monotonic()
            t = now - started
            # Gentle synthetic motion. Position is metres; quaternion x,y,z,w.
            pos = (0.02 * math.sin(t), 1.65, 0.03 * math.cos(t * 0.7))
            yaw = 0.12 * math.sin(t * 0.4)
            quat = (0.0, math.sin(yaw / 2), 0.0, math.cos(yaw / 2))
            tracking = TRACKING_PAYLOAD.pack(
                time.time_ns(), *pos, *quat,
                0.02 * math.cos(t), 0.0, -0.02 * math.sin(t * 0.7),
                0.0, 0.0, 0.0, 0x3,
            )
            seq = (seq + 1) & 0xFFFFFFFF
            sock.sendto(packet(TRACKING, seq, tracking), address)

            # Touch-like simulated trigger values and occasional button mask changes.
            for hand in (0, 1):
                x = (-0.25 if hand == 0 else 0.25) + 0.01 * math.sin(t + hand)
                y = 1.25 + 0.01 * math.cos(t * 0.8 + hand)
                z = -0.45
                cquat = (0.0, 0.0, 0.0, 1.0)
                trigger = (math.sin(t * 0.5 + hand) + 1.0) * 0.5
                grip = (math.cos(t * 0.3 + hand) + 1.0) * 0.5
                stick = (0.25 * math.sin(t + hand), 0.25 * math.cos(t + hand))
                buttons = 1 if int(t * 0.5 + hand) % 8 == 0 else 0
                controller = CONTROLLER_PAYLOAD.pack(
                    time.time_ns(), hand, 0x3, 0, buttons, trigger, grip,
                    *stick, x, y, z, *cquat,
                )
                seq = (seq + 1) & 0xFFFFFFFF
                sock.sendto(packet(CONTROLLER, seq, controller), address)

            next_tick += interval
            time.sleep(max(0.0, next_tick - time.monotonic()))
    except KeyboardInterrupt:
        print('\nSimulator stopped.')
    finally:
        try:
            seq = (seq + 1) & 0xFFFFFFFF
            sock.sendto(packet(5, seq, struct.pack('<Q', time.time_ns())), address)
        except OSError:
            pass
        sock.close()


if __name__ == '__main__':
    main()
