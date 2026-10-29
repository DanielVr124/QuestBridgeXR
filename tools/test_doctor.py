#!/usr/bin/env python3
"""Tests for protocol-level setup diagnostics."""
from __future__ import annotations

import socket
import threading
import unittest

import doctor


class DoctorTests(unittest.TestCase):
    def test_hello_packet_matches_shared_wire_format(self):
        packet = doctor.make_hello(42, "Quest Test")
        magic, version, packet_type, sequence, payload_bytes = doctor.HEADER.unpack_from(packet)
        self.assertEqual((magic, version, packet_type, sequence),
                         (doctor.MAGIC, doctor.PROTOCOL_VERSION, doctor.TYPE_HELLO, 42))
        self.assertEqual(payload_bytes, doctor.HELLO.size)
        client_version, capabilities, name = doctor.HELLO.unpack_from(packet, doctor.HEADER.size)
        self.assertEqual(client_version, 1)
        self.assertEqual(capabilities, 0x7)
        self.assertTrue(name.startswith(b"Quest Test"))

    def test_hello_ack_is_validated(self):
        sequence = 12345
        payload = doctor.HELLO_ACK.pack(1, 0x7, 9944, 0)
        packet = doctor.HEADER.pack(doctor.MAGIC, doctor.PROTOCOL_VERSION,
                                    doctor.TYPE_HELLO_ACK, sequence, len(payload)) + payload
        self.assertEqual(doctor.parse_hello_ack(packet, sequence), (1, 0x7))

    def test_probe_host_completes_real_local_udp_handshake(self):
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as server:
            server.bind(("127.0.0.1", 0))
            port = server.getsockname()[1]
            server.settimeout(1.0)

            def serve_once():
                data, peer = server.recvfrom(2048)
                _magic, _version, _kind, sequence, _payload_size = doctor.HEADER.unpack_from(data)
                payload = doctor.HELLO_ACK.pack(1, 0x7, port, 0)
                reply = doctor.HEADER.pack(doctor.MAGIC, doctor.PROTOCOL_VERSION,
                                           doctor.TYPE_HELLO_ACK, sequence, len(payload)) + payload
                server.sendto(reply, peer)

            thread = threading.Thread(target=serve_once, daemon=True)
            thread.start()
            config = {"network": {"pc_ip": "127.0.0.1", "host_port": port}}
            message = doctor.probe_host(config, timeout_seconds=1.0)
            thread.join(timeout=1.0)
        self.assertIn("HELLO_ACK received", message)

    def test_hello_ack_rejects_wrong_type_and_bad_length(self):
        sequence = 9
        payload = doctor.HELLO_ACK.pack(1, 0, 9944, 0)
        wrong_type = doctor.HEADER.pack(doctor.MAGIC, doctor.PROTOCOL_VERSION,
                                        doctor.TYPE_HELLO, sequence, len(payload)) + payload
        with self.assertRaisesRegex(ValueError, "expected HELLO_ACK"):
            doctor.parse_hello_ack(wrong_type, sequence)
        truncated = doctor.HEADER.pack(doctor.MAGIC, doctor.PROTOCOL_VERSION,
                                       doctor.TYPE_HELLO_ACK, sequence, 4) + b"1234"
        with self.assertRaisesRegex(ValueError, "wrong size"):
            doctor.parse_hello_ack(truncated, sequence)


if __name__ == "__main__":
    unittest.main(verbosity=2)
