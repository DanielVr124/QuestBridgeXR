#!/usr/bin/env python3
"""Tests for the framed TCP transport used by USB/ADB streaming."""
from __future__ import annotations

import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from stream_test_video import (
    HEADER,
    MAX_DATAGRAM,
    frame_tcp_packet,
    make_packet,
    pop_tcp_packets,
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


if __name__ == "__main__":
    unittest.main(verbosity=2)
