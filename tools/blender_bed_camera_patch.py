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
_CYCLES_SAMPLES = re.compile(r"scene\.cycles\.samples\s*=\s*\d+")
_RENDER_RESOLUTION = re.compile(
    r"(?m)^scene\.render\.resolution_x\s*=\s*\d+;\s*"
    r"scene\.render\.resolution_y\s*=\s*\d+;\s*"
    r"scene\.render\.resolution_percentage\s*=\s*\d+\s*$"
)

# The imported glTF rotates Y-up into Blender Z-up. The headboard is on +Y;
# its upholstered face looks toward the mattress/foot side (-Y). These are
# offline Blender review cameras only; runtime VR camera/tracking is untouched.
# Keep camera names stable: the source-packaging job expects exact render filenames.
# This emission override exists only in the offline Blender review scene; the
# headset continues to reveal the LED through its normal view-angle / zone logic.
_PREVIEW_LED_SETUP = """# Enable the connected headboard strip in all offline Blender review angles.
for _qbx_led_mat in bpy.data.materials:
    _qbx_name = _qbx_led_mat.name.lower()
    if 'led diffuser' not in _qbx_name and 'headboard_led_diffuser' not in _qbx_name:
        continue
    _qbx_led_mat.diffuse_color = (0.055, 0.14, 1.0, 1.0)
    _qbx_led_mat.use_nodes = True
    _qbx_bsdf = next((n for n in _qbx_led_mat.node_tree.nodes
                      if n.type == 'BSDF_PRINCIPLED'), None)
    if _qbx_bsdf is not None:
        _qbx_emission = _qbx_bsdf.inputs.get('Emission Color')
        if _qbx_emission is not None:
            _qbx_emission.default_value = (0.055, 0.14, 1.0, 1.0)
        _qbx_strength = _qbx_bsdf.inputs.get('Emission Strength')
        if _qbx_strength is not None:
            _qbx_strength.default_value = 4.5
"""

NEW_VIEWS = """review_views = [
    # Wide three-quarter framing of the full platform, mattress and headboard.
    ('bed_three_quarter', (5.5,-4.3,4.0), (0,0,1.0), 42),
    # Front view from the mattress/foot side, looking toward the upholstered headboard.
    ('bed_headboard', (0.0,-3.70,2.10), (0,1.22,1.48), 48),
    # Closer front detail of the diamond insert, mirror bevels and crystal hardware.
    ('bed_headboard_detail', (0.35,-2.65,1.92), (0,1.22,1.52), 62),
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
        replacement = "scene.cycles.samples=16\nscene.cycles.use_denoising=True"
        patched = patched[:sample.start()] + replacement + patched[sample.end():]

    resolution_matches = list(_RENDER_RESOLUTION.finditer(patched))
    if len(resolution_matches) > 1:
        raise RuntimeError("Blender camera patch expected at most one render resolution setting.")
    if resolution_matches:
        resolution = resolution_matches[0]
        replacement = (
            "scene.render.resolution_x=1200; scene.render.resolution_y=900; "
            "scene.render.resolution_percentage=100"
        )
        patched = patched[:resolution.start()] + replacement + patched[resolution.end():]

    # Brighten the headboard LED only in this offline Blender preview scene.
    # The runtime GLB keeps its low-level diffuser material and native angle gate.
    import_marker = "bpy.ops.import_scene.gltf(filepath=str(args.source))\n"
    import_count = patched.count(import_marker)
    if import_count > 1:
        raise RuntimeError("Blender preview LED patch found multiple glTF import points.")
    if import_count == 1:
        patched = patched.replace(import_marker, import_marker + _PREVIEW_LED_SETUP, 1)

    return patched
