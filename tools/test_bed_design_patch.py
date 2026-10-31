#!/usr/bin/env python3
"""Contract checks for the revised headboard and propped sleep pillows."""
from __future__ import annotations
import gzip
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from bed_design_patch import patch_bed_geometry

def main() -> None:
    compressed = ROOT / "tools" / "generate_bed_asset.py.gz"
    source = gzip.decompress(compressed.read_bytes()).decode("utf-8")
    patched = patch_bed_geometry(source)
    required = (
        "Headboard_full_width_padded_face",
        "Headboard_wrap_wing_",
        "Sleep_pillow_front_stitch_",
        "PILLOW_TILT = math.radians(22.0)",
        "Wide sculpted charcoal wingback headboard",
    )
    for token in required:
        assert token in patched, f"missing revised bed geometry: {token}"
    forbidden = (
        "Headboard_channel_",
        "Slate_accent_cushion_",
        "Lumbar_cushion",
    )
    for token in forbidden:
        assert token not in patched, f"old bed component remains: {token}"
    print("Bed geometry patch contract passed.")

if __name__ == "__main__":
    main()
