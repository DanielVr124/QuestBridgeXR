"""Robust review-camera framing for the packed Blender bed scene.

The original importer is gzip-packed, so its view coordinates can change without
requiring a brittle whole-block string match. We replace only the review_views
literal after verifying that it contains the four expected bed views.
"""
from __future__ import annotations

import re

_REVIEW_VIEWS = re.compile(r"(?ms)^review_views\\s*=\\s*\\[.*?^\\]")
_REQUIRED_VIEWS = {
    "bed_three_quarter",
    "bed_headboard",
    "bed_side_profile",
    "bed_foot_three_quarter",
}

# The imported glTF rotates Y-up into Blender Z-up. The bed's length becomes
# Blender Y: the headboard is on +Y and the footboard is on -Y.
NEW_VIEWS = """review_views = [
    # Wide three-quarter framing of the full platform, mattress and headboard.
    ('bed_three_quarter', (5.5,-4.3,4.0), (0,0,1.0), 42),
    # Headboard-facing view from its +Y side, with the tufting kept near eye level.
    ('bed_headboard', (3.3,4.25,2.8), (0,1.2,1.25), 52),
    # Close fabric view of the domed pillows, grey welt and deep box gusset.
    ('bed_pillow_detail', (2.0,2.8,1.95), (0,.82,1.12), 68),
    # Side silhouette view shows pillow depth, mattress height and frame trim.
    ('bed_side_profile', (6.5,0.0,2.65), (0,0,1.0), 48),
    # Footboard detail from the -Y side.
    ('bed_foot_three_quarter', (-5.5,-4.2,4.0), (0,-1.1,.9), 42),
]"""


def patch_blender_review_cameras(source: str) -> str:
    matches = list(_REVIEW_VIEWS.finditer(source))
    if not matches:
        raise RuntimeError("Blender camera patch expected a review_views list.")
    if len(matches) != 1:
        raise RuntimeError("Blender camera patch expected exactly one review_views list.")

    match = matches[0]
    original = match.group(0)
    present = set(re.findall(r"\\(\\s*['\\\"]([^'\\\"]+)['\\\"]\\s*,", original))
    missing = _REQUIRED_VIEWS - present
    if missing:
        names = ", ".join(sorted(missing))
        raise RuntimeError(
            f"Blender camera patch review_views list is missing required bed camera entries: {names}"
        )

    return source[:match.start()] + NEW_VIEWS + source[match.end():]
