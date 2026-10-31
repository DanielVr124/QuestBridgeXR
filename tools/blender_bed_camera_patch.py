"""Correct camera framing and subject direction in the packed Blender import scene."""
from __future__ import annotations

import re

NEW_VIEWS = """review_views = [
    # Wide framing keeps the whole bed footprint inside the first review image.
    ('bed_three_quarter', (5.5,4.3,6.2), (0,.95,0), 42),
    # The upholstered face points into +Z; these cameras stay on that front side.
    ('bed_headboard', (2.8,2.7,4.8), (0,1.28,-1.28), 52),
    # Dedicated fabric/pillow close-up, framed around both puffed cushions.
    ('bed_pillow_detail', (1.9,2.4,3.1), (0,1.12,-1.02), 68),
    ('bed_side_profile', (6.5,2.7,.2), (0,.95,0), 48),
    ('bed_foot_three_quarter', (-5.5,4.0,6.2), (0,.9,.1), 42),
]"""

_REVIEW_VIEWS = re.compile(
    r"(?ms)^(?P<indent>[ \t]*)review_views[ \t]*=[ \t]*\[.*?^[ \t]*\]"
)
_REQUIRED_CAMERAS = (
    "bed_three_quarter",
    "bed_headboard",
    "bed_side_profile",
    "bed_foot_three_quarter",
)


def patch_blender_review_cameras(source: str) -> str:
    matches = list(_REVIEW_VIEWS.finditer(source))
    if len(matches) != 1:
        raise RuntimeError(
            "Blender camera patch expected exactly one review_views list; "
            "refusing to render misleading close-up or back-facing camera angles."
        )
    match = matches[0]
    original = match.group(0)
    if not all(camera in original for camera in _REQUIRED_CAMERAS):
        raise RuntimeError(
            "Blender camera patch found a review_views list without the required bed camera entries."
        )
    indent = match.group("indent")
    updated = NEW_VIEWS
    if indent:
        updated = "\n".join(indent + line if line else line for line in NEW_VIEWS.splitlines())
    return source[:match.start()] + updated + source[match.end():]
