#!/usr/bin/env python3
"""Launch the packed Blender scene/import script with corrected reviewer cameras."""
from __future__ import annotations
import gzip
from pathlib import Path
from blender_bed_camera_patch import patch_blender_review_cameras

compressed = Path(__file__).with_suffix(".py.gz")
source = gzip.decompress(compressed.read_bytes()).decode("utf-8")
source = patch_blender_review_cameras(source)
namespace = {"__name__": "__main__", "__file__": str(compressed), "__package__": None}
exec(compile(source, str(compressed), "exec"), namespace)
