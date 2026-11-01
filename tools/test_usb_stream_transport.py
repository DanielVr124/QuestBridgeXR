#!/usr/bin/env python3
"""Tests for the framed TCP transport used by USB/ADB streaming."""
from __future__ import annotations

import pathlib
import socket
import sys
import threading
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from stream_test_video import (
    HEADER,
    MAX_DATAGRAM,
    frame_tcp_packet,
    make_packet,
    pop_tcp_packets,
    usb_control_relay,
)


class UsbStreamTransportTests(unittest.TestCase):
    def test_round_trip_preserves_existing_qbx_packet(self):
        packet = make_packet(8, 12, b"video-config-payload")
        framed = frame_tcp_packet(packet)
        pending = bytearray(framed)
        self.assertEqual(pop_tcp_packets(pending), [packet])
        self.assertEqual(pending, bytearray())

    def test_parser_handles_split_tcp_reads(self):
        packet = make_packet(6, 99, b"frame-chunk-data")
        framed = frame_tcp_packet(packet)
        pending = bytearray(framed[:3])
        self.assertEqual(pop_tcp_packets(pending), [])
        pending.extend(framed[3:11])
        self.assertEqual(pop_tcp_packets(pending), [])
        pending.extend(framed[11:])
        self.assertEqual(pop_tcp_packets(pending), [packet])

    def test_parser_handles_multiple_packets_in_one_tcp_read(self):
        packets = [
            make_packet(8, 1, b"config"),
            make_packet(6, 2, b"chunk-a"),
            make_packet(6, 3, b"chunk-b"),
        ]
        pending = bytearray().join(bytearray(frame_tcp_packet(p)) for p in packets)
        self.assertEqual(pop_tcp_packets(pending), packets)
        self.assertEqual(pending, bytearray())

    def test_rejects_packet_smaller_than_qbx_header(self):
        with self.assertRaisesRegex(ValueError, "outside"):
            frame_tcp_packet(b"short")

    def test_rejects_packet_larger_than_udp_compatible_qbx_limit(self):
        packet = bytes(MAX_DATAGRAM + 1)
        with self.assertRaisesRegex(ValueError, "outside"):
            frame_tcp_packet(packet)

    def test_parser_rejects_invalid_stream_length_prefix(self):
        pending = bytearray((MAX_DATAGRAM + 1).to_bytes(4, "little"))
        with self.assertRaisesRegex(ValueError, "invalid QBX TCP frame length"):
            pop_tcp_packets(pending)

    def test_packet_length_prefix_is_little_endian_and_exact(self):
        packet = make_packet(8, 10, b"x")
        framed = frame_tcp_packet(packet)
        self.assertEqual(int.from_bytes(framed[:4], "little"), len(packet))
        self.assertEqual(framed[4:], packet)
        self.assertEqual(len(packet), HEADER.size + 1)

    def test_bidirectional_usb_relay_bridges_quest_packets_and_host_replies(self):
        listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        listener.bind(("127.0.0.1", 0))
        listener.listen(1)
        quest = socket.create_connection(listener.getsockname(), timeout=2)
        pc_client, _ = listener.accept()
        quest.settimeout(2)
        listener.close()

        relay = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        relay.bind(("127.0.0.1", 0))
        relay.setblocking(False)
        host = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        host.bind(("127.0.0.1", 0))
        host.settimeout(2)

        stop = threading.Event()
        send_lock = threading.Lock()
        relay_thread = threading.Thread(
            target=usb_control_relay,
            args=(pc_client, relay, host.getsockname(), send_lock, stop),
            daemon=True,
        )
        relay_thread.start()
        outbound = make_packet(1, 21, b"quest-hello")
        response = make_packet(2, 22, b"windows-host-ack")
        try:
            quest.sendall(frame_tcp_packet(outbound))
            received, relay_peer = host.recvfrom(2048)
            self.assertEqual(received, outbound)
            host.sendto(response, relay_peer)

            pending = bytearray()
            replies = []
            while not replies:
                pending.extend(quest.recv(4096))
                replies = pop_tcp_packets(pending)
            self.assertEqual(replies, [response])
        finally:
            stop.set()
            quest.close()
            pc_client.close()
            relay.close()
            host.close()
            relay_thread.join(timeout=2)


if __name__ == "__main__":
    unittest.main(verbosity=2)
