"""Targeted geometry update for the editable upholstered bed generator.

The original generator is gzip-packed to keep the repository compact. This patch
is deliberately isolated in plain Python so the bed design remains reviewable.
"""
from __future__ import annotations

OLD_GEOMETRY = "# ----------------------- PILLOWS AND HEADBOARD -----------------------\n# Two substantial sleep pillows plus smaller slate accent cushions, each independently modeled.\nfor side in (-1,1):\n    x=side*.49\n    superellipsoid(f'Sleep_pillow_{side}',(x,1.095,-1.115),(.52,.175,.335),'sheet',.48,72,44,taper=.34)\n    # Fine seam along the pillow's visible front crown.\n    curve_tube(f'Sleep_pillow_piping_{side}',[(x+u,1.19,-1.405+.025*math.cos(u*2.2)) for u in np.linspace(-.43,.43,28)],.0024,'pillow')\nfor side in (-1,1):\n    x=side*.285\n    superellipsoid(f'Slate_accent_cushion_{side}',(x,1.215,-.795),(.37,.155,.245),'pillow',.50,64,40,taper=.26)\n# A compact lumbar cushion in muted steel blue to layer the pillows.\nsuperellipsoid('Lumbar_cushion',(0,1.245,-.57),(.46,.135,.155),'throw',.53,64,36,taper=.30)\n\n# Structural headboard shell and seven individual, vertically channeled upholstered pads.\n# The channels have eased shoulders, a softly bowed face, and visible shadow seams.\nsuperellipsoid('Headboard_back_shell',(0,1.185,-1.575),(1.29,.925,.115),'charcoal2',.50,88,52)\nsuperellipsoid('Headboard_top_rail',(0,2.035,-1.475),(1.245,.085,.125),'charcoal',.55,72,36)\nfor i in range(7):\n    x=(i-3)*.354\n    superellipsoid(f'Headboard_channel_{i+1:02d}',(x,1.275,-1.435),(.174,.735,.112),'charcoal',.50,64,44,taper=.18)\n    # Delicate stitched inset down the centre of every padded section.\n    curve_tube(f'Headboard_channel_stitch_{i+1:02d}',[(x,0.72+j*.043,-1.326+.006*math.cos(j*.18)) for j in range(29)],.00115,'charcoal2')\n# Side wings roll forward just a little, framing the upholstered centre panel.\nfor side in (-1,1):\n    superellipsoid(f'Headboard_side_bolster_{side}',(side*1.205,1.255,-1.415),(.105,.77,.135),'charcoal2',.50,56,36)\n# Rear support brackets are hidden in normal view but add a credible connection to the frame.\nfor side in (-1,1):\n    boxsoft(f'Headboard_mount_{side}',(side*.92,.74,-1.53),(.105,.48,.13),'underside',.40)\n"

NEW_GEOMETRY = "# ----------------------- PILLOWS AND HEADBOARD -----------------------\n# Refresh only the pillows: softly filled fabric forms, not hard-edged blocks.\nfor side in (-1,1):\n    x=side*.49\n    superellipsoid(\n        f'Sleep_pillow_{side}',(x,1.095,-1.115),\n        (.535,.165,.335),'sheet',.54,88,56,taper=.24\n    )\n    # Fine welt follows the pillow's tucked rear edge.\n    curve_tube(\n        f'Sleep_pillow_piping_{side}',\n        [(x+u,1.188,-1.405+.022*math.cos(u*2.2)) for u in np.linspace(-.44,.44,36)],\n        .0022,'pillow'\n    )\n    # Two very shallow corner tension folds give the fabric a naturally filled look.\n    for flank in (-1,1):\n        curve_tube(\n            f'Sleep_pillow_corner_fold_{side}_{flank}',\n            [(x+flank*.43,1.145,-1.407),\n             (x+flank*.475,1.165,-1.385),\n             (x+flank*.49,1.176,-1.352)],\n            .00125,'pillow',sides=6\n        )\n\nfor side in (-1,1):\n    x=side*.285\n    superellipsoid(\n        f'Slate_accent_cushion_{side}',(x,1.215,-.795),\n        (.355,.145,.235),'pillow',.56,80,52,taper=.22\n    )\n    curve_tube(\n        f'Slate_accent_piping_{side}',\n        [(x-.285,1.245,-.985),(x,1.278,-1.015),(x+.285,1.245,-.985)],\n        .0017,'throw',sides=6\n    )\n\n# A low lumbar pillow remains in its original position and material family.\nsuperellipsoid(\n    'Lumbar_cushion',(0,1.245,-.57),\n    (.445,.125,.145),'throw',.56,80,48,taper=.20\n)\ncurve_tube(\n    'Lumbar_cushion_seam',\n    [(-.36,1.292,-.638),(0,1.315,-.642),(.36,1.292,-.638)],\n    .0015,'pillow',sides=6\n)\n\n# ----------------------- RESTORED BED + TARGETED HEADBOARD UPDATE -----------------------\n# Keep the original bed platform, rails, legs, foot end, mattress, fitted sheet,\n# duvet and throw untouched. Only the headboard upholstery and pillows are replaced.\nsuperellipsoid(\n    'Headboard_back_shell',(0,1.185,-1.575),\n    (1.29,.925,.115),'charcoal2',.50,88,52\n)\nboxsoft(\n    'Headboard_quilt_backing',(0,1.175,-1.425),\n    (2.42,1.56,.15),'charcoal2',.40\n)\nsuperellipsoid(\n    'Headboard_top_rail',(0,2.035,-1.475),\n    (1.245,.085,.125),'charcoal2',.55,80,44\n)\nfor side in (-1,1):\n    superellipsoid(\n        f'Headboard_side_bolster_{side}',\n        (side*1.205,1.255,-1.415),\n        (.105,.77,.135),'charcoal2',.54,72,44,taper=.10\n    )\n\n# Five broad padded sections with a restrained radius: softened filling,\n# but still visibly rectangular rather than oversized round cushions.\nfor i in range(5):\n    x=(i-2)*.49\n    superellipsoid(\n        f'Headboard_filled_panel_{i+1:02d}',\n        (x,1.275,-1.435),\n        (.225,.705,.112),'charcoal',.54,88,56,taper=.10\n    )\n\n# Nested upholstered lips shade the recessed RGB emitter from reclined angles.\n# The inner and outer lips create a narrow channel; the strip is not exposed tape.\nfor side in (-1,1):\n    superellipsoid(\n        f'Headboard_outer_lip_side_{side}',\n        (side*1.365,1.185,-1.205),\n        (.045,.835,.105),'charcoal2',.48,72,44\n    )\n    superellipsoid(\n        f'Headboard_inner_lip_side_{side}',\n        (side*1.155,1.185,-1.205),\n        (.040,.745,.105),'charcoal',.52,72,44\n    )\n\nfor name,y in (('top',2.115),('bottom',.245)):\n    superellipsoid(\n        f'Headboard_outer_lip_edge_{name}',\n        (0,y,-1.205),\n        (1.365,.045,.105),'charcoal2',.48,88,40\n    )\nfor name,y in (('top',1.845),('bottom',.525)):\n    superellipsoid(\n        f'Headboard_inner_lip_edge_{name}',\n        (0,y,-1.205),\n        (1.155,.040,.105),'charcoal',.52,88,40\n    )\n\n# Original support brackets are retained exactly.\nfor side in (-1,1):\n    boxsoft(f'Headboard_mount_{side}',(side*.92,.74,-1.53),(.105,.48,.13),'underside',.40)\n"

def patch_bed_geometry(source: str) -> str:
    if source.count(OLD_GEOMETRY) != 1:
        raise RuntimeError(
            "Bed geometry patch did not find exactly one original pillow/headboard block; "
            "refusing to generate an unexpected model."
        )
    source = source.replace(OLD_GEOMETRY, NEW_GEOMETRY, 1)

    # Trimesh's glTF exporter only writes NORMAL accessors when vertex_normals
    # have already been evaluated and cached. Without this, the diamond pads
    # become faceted flat-shaded triangles in the runtime and editable GLBs.
    smooth_mesh_marker = "        m.fix_normals()\n"
    if source.count(smooth_mesh_marker) != 1:
        raise RuntimeError("Bed mesh smooth-normal marker changed; refusing a flat-shaded GLB.")
    source = source.replace(
        smooth_mesh_marker,
        smooth_mesh_marker + "        m.vertex_normals  # cache smooth per-vertex normals for GLB export\n",
        1
    )
    runtime_mesh_marker = "    merged.fix_normals()\n"
    if source.count(runtime_mesh_marker) != 1:
        raise RuntimeError("Runtime mesh normal marker changed; refusing a flat-shaded runtime GLB.")
    source = source.replace(
        runtime_mesh_marker,
        runtime_mesh_marker + "    merged.vertex_normals  # include normals in optimized runtime batches\n",
        1
    )

    old_design = "'design':'Charcoal channel-upholstered headboard, padded upholstered rails, silver cotton bedding, graphite duvet, midnight ribbed throw',"
    new_design = "'design':'Original bed platform, frame, mattress and bedding retained; five softly padded charcoal headboard sections with gently eased rectangular edges, a nested recessed RGB perimeter channel and refreshed fabric pillows',"
    if source.count(old_design) != 1:
        raise RuntimeError("Bed metadata design marker changed; update the design patch.")
    source = source.replace(old_design, new_design, 1)
    return source
