#!/usr/bin/env python3
"""Launch the packed bed generator after applying its readable geometry update."""
from __future__ import annotations
import gzip
from pathlib import Path
from bed_design_patch import patch_bed_geometry

compressed = Path(__file__).with_suffix(".py.gz")
source = gzip.decompress(compressed.read_bytes()).decode("utf-8")
source = patch_bed_geometry(source)
namespace = {"__name__": "__main__", "__file__": str(compressed), "__package__": None}
exec(compile(source, str(compressed), "exec"), namespace)
