#!/usr/bin/env python3
"""Launch the packed Blender scene/import script stored beside this wrapper."""
from __future__ import annotations
import gzip
from pathlib import Path

compressed = Path(__file__).with_suffix(".py.gz")
source = gzip.decompress(compressed.read_bytes())
namespace = {"__name__": "__main__", "__file__": str(compressed), "__package__": None}
exec(compile(source, str(compressed), "exec"), namespace)
