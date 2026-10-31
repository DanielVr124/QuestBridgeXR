"""Robust review-camera framing and render settings for the packed Blender bed scene.

The original importer is gzip-packed, so its view coordinates can change without
requiring a brittle whole-block string match. This patch refuses to render
misleading close-up or back-facing camera angles unless the expected bed views
are present in the source list. It also uses denoised preview settings so the
five inspection renders stay detailed without taking excessively long.
"""
from __future__ import annotations

import re

_REVIEW_VIEWS = re.compile(r"(?ms)^review_views\s*=\s*\[.*?\]")
_REQUIRED_VIEWS = {
    "bed_three_quarter",
    "bed_headboard",
    "bed_side_profile",
    "bed_foot_three_quarter",
}
_CYCLES_SAMPLES = re.compile(r"(?m)^scene\.cycles\.samples\s*=\s*\d+\s*$")
_RENDER_RESOLUTION = re.compile(
    r"(?m)^scene\.render\.resolution_x\s*=\s*\d+;\s*"
    r"scene\.render\.resolution_y\s*=\s*\d+;\s*"
    r"scene\.render\.resolution_percentage\s*=\s*\d+\s*$"
)

# The imported glTF rotates Y-up into Blender Z-up. The bed's length becomes
# Blender Y: the headboard is on +Y and the footboard is on -Y.
# Keep camera names stable: the source-packaging job expects exact render filenames.
NEW_VIEWS = """review_views = [
    # Wide three-quarter framing of the full platform, mattress and headboard.
    ('bed_three_quarter', (5.5,-4.3,4.0), (0,0,1.0), 42),
    # Headboard-facing view from its +Y side, with the tufting kept near eye level.
    ('bed_headboard', (3.3,4.25,2.8), (0,1.2,1.25), 52),
    # Close fabric view from the headboard end: domed crowns, grey welt and full box-gusset depth.
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
    present = set(re.findall(r"\(\s*'([^']+)'\s*,", original))
    missing = _REQUIRED_VIEWS - present
    if missing:
        names = ", ".join(sorted(missing))
        raise RuntimeError(
            f"Blender camera patch review_views list is missing required bed camera entries: {names}"
        )

    patched = source[:match.start()] + NEW_VIEWS + source[match.end():]

    # Denoised 24-sample, 1440x1080 Cycles previews retain enough resolution
    # to judge stitching, tufting and pillow welt while cutting render time.
    sample_matches = list(_CYCLES_SAMPLES.finditer(patched))
    if len(sample_matches) > 1:
        raise RuntimeError("Blender camera patch expected at most one Cycles sample setting.")
    if sample_matches:
        sample = sample_matches[0]
        replacement = "scene.cycles.samples=24\nscene.cycles.use_denoising=True"
        patched = patched[:sample.start()] + replacement + patched[sample.end():]

    resolution_matches = list(_RENDER_RESOLUTION.finditer(patched))
    if len(resolution_matches) > 1:
        raise RuntimeError("Blender camera patch expected at most one render resolution setting.")
    if resolution_matches:
        resolution = resolution_matches[0]
        replacement = (
            "scene.render.resolution_x=1440; scene.render.resolution_y=1080; "
            "scene.render.resolution_percentage=100"
        )
        patched = patched[:resolution.start()] + replacement + patched[resolution.end():]

    return patched
