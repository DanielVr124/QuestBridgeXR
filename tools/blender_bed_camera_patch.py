"""Correct camera framing and subject direction in the packed Blender import scene."""
from __future__ import annotations

OLD_VIEWS = """review_views = [
    ('bed_three_quarter', (4.4,3.1,4.0), (0,.95,0), 52),
    ('bed_headboard', (3.5,2.45,-3.25), (0,1.32,-1.28), 58),
    ('bed_side_profile', (4.9,2.35,.15), (0,.82,0), 54),
    ('bed_foot_three_quarter', (-4.0,2.8,2.8), (0,.9,.1), 52),
]"""

NEW_VIEWS = """review_views = [
    # Wide framing keeps the whole 3.75 m bed inside the first review image.
    ('bed_three_quarter', (5.5,4.3,6.2), (0,.95,0), 42),
    # The upholstered face points into +Z; these cameras stay on that front side.
    ('bed_headboard', (2.8,2.7,4.8), (0,1.28,-1.28), 52),
    # Dedicated fabric/pillow close-up, framed around both puffed cushions.
    ('bed_pillow_detail', (1.9,2.4,3.1), (0,1.12,-1.02), 68),
    ('bed_side_profile', (6.5,2.7,.2), (0,.95,0), 48),
    ('bed_foot_three_quarter', (-5.5,4.0,6.2), (0,.9,.1), 42),
]"""


def patch_blender_review_cameras(source: str) -> str:
    if source.count(OLD_VIEWS) != 1:
        raise RuntimeError(
            "Blender camera patch did not find the expected review_views block; "
            "refusing to render misleading close-up or back-facing camera angles."
        )
    return source.replace(OLD_VIEWS, NEW_VIEWS, 1)
