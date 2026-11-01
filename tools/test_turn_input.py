#!/usr/bin/env python3
"""Guard the Quest room/city snap-turn input threshold against regressions."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
RENDERER = ROOT / "quest-client/app/src/main/cpp/room_renderer.cpp"


class TurnInputTests(unittest.TestCase):
    def test_snap_turn_uses_one_reachable_threshold_for_room_and_city(self):
        source = RENDERER.read_text(encoding="utf-8")
        self.assertEqual(source.count("constexpr float kSnapTurnThreshold = 0.45f;"), 1)
        self.assertEqual(source.count("rightStickX>kSnapTurnThreshold,left=rightStickX < -kSnapTurnThreshold"), 2)

    def test_turn_direction_is_consistent_between_room_and_city(self):
        source = RENDERER.read_text(encoding="utf-8")
        self.assertEqual(source.count("virtualYaw_-=settings_.snapTurnDegrees*kPi/180.0f"), 2)
        self.assertEqual(source.count("virtualYaw_+=settings_.snapTurnDegrees*kPi/180.0f"), 2)


if __name__ == "__main__":
    unittest.main(verbosity=2)
