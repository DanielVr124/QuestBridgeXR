#!/usr/bin/env python3
"""Unit tests for safe LAN address parsing used by Quick Network Setup."""
from __future__ import annotations

import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from network_setup import parse_lan_ipv4


class NetworkSetupTests(unittest.TestCase):
    def test_skips_loopback_and_link_local_addresses(self):
        output = (
            "2: lo: <LOOPBACK>\n    inet 127.0.0.1/8 scope host lo\n"
            "3: wlan0: <UP>\n    inet 169.254.10.2/16 scope link wlan0\n"
            "    inet 192.168.1.42/24 scope global wlan0\n"
        )
        self.assertEqual(parse_lan_ipv4(output), "192.168.1.42")

    def test_supports_route_output_and_ignores_invalid_values(self):
        self.assertEqual(parse_lan_ipv4("1.1.1.1 via 192.168.1.1 dev wlan0 src 10.0.0.25"), "10.0.0.25")
        self.assertIsNone(parse_lan_ipv4("inet 0.0.0.0/32\ninet 224.0.0.9/32"))


if __name__ == "__main__":
    unittest.main(verbosity=2)
