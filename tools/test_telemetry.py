#!/usr/bin/env python3
"""Unit tests for the loopback telemetry packet monitor."""
from __future__ import annotations

import struct
import unittest

import monitor_telemetry as telemetry


class TelemetryTests(unittest.TestCase):
    def test_decode_tracking_payload(self):
        payload = telemetry.TRACKING.pack(
            123456, 1.0, 2.0, 3.0, 0.0, 0.0, 0.0, 1.0,
            0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 3,
        )
        parsed = telemetry.decode_tracking(payload)
        self.assertEqual(parsed["timestamp_ns"], 123456)
        self.assertEqual(parsed["position"], (1.0, 2.0, 3.0))
        self.assertEqual(parsed["flags"], 3)

    def test_decode_controller_payload(self):
        payload = telemetry.CONTROLLER.pack(
            123, 1, 3, 0, 5, 0.75, 0.25, 0.1, -0.2,
            4.0, 5.0, 6.0, 0.0, 0.0, 0.0, 1.0,
        )
        parsed = telemetry.decode_controller(payload)
        self.assertEqual(parsed["hand"], 1)
        self.assertEqual(parsed["buttons"], 5)
        self.assertAlmostEqual(parsed["thumbstick"][0], 0.1, places=6)
        self.assertAlmostEqual(parsed["thumbstick"][1], -0.2, places=6)

    def test_decode_packet_valid_header_and_payload_size(self):
        payload = telemetry.TRACKING.pack(0, *([0.0] * 13), 3)
        header = telemetry.HEADER.pack(
            telemetry.MAGIC, telemetry.PROTOCOL_VERSION, telemetry.TYPE_TRACKING, 77, len(payload)
        )
        packet_type, sequence, body = telemetry.decode_packet(header + payload)
        self.assertEqual(packet_type, telemetry.TYPE_TRACKING)
        self.assertEqual(sequence, 77)
        self.assertEqual(body, payload)

    def test_bad_magic_is_rejected(self):
        payload = b""
        datagram = telemetry.HEADER.pack(0, telemetry.PROTOCOL_VERSION, 3, 1, len(payload))
        with self.assertRaisesRegex(ValueError, "magic"):
            telemetry.decode_packet(datagram)

    def test_bad_payload_size_is_rejected(self):
        datagram = telemetry.HEADER.pack(
            telemetry.MAGIC, telemetry.PROTOCOL_VERSION, telemetry.TYPE_TRACKING, 1, 4
        ) + b"1234"
        with self.assertRaisesRegex(ValueError, "TRACKING payload length"):
            telemetry.decode_packet(datagram)


if __name__ == "__main__":
    unittest.main()
