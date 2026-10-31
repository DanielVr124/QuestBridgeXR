"""Targeted geometry update for the editable upholstered bed generator.

The original generator is gzip-packed to keep the repository compact. This patch
is deliberately isolated in plain Python so the bed design remains reviewable.
"""
from __future__ import annotations

OLD_GEOMETRY = "# ----------------------- PILLOWS AND HEADBOARD -----------------------\n# Two substantial sleep pillows plus smaller slate accent cushions, each independently modeled.\nfor side in (-1,1):\n    x=side*.49\n    superellipsoid(f'Sleep_pillow_{side}',(x,1.095,-1.115),(.52,.175,.335),'sheet',.48,72,44,taper=.34)\n    # Fine seam along the pillow's visible front crown.\n    curve_tube(f'Sleep_pillow_piping_{side}',[(x+u,1.19,-1.405+.025*math.cos(u*2.2)) for u in np.linspace(-.43,.43,28)],.0024,'pillow')\nfor side in (-1,1):\n    x=side*.285\n    superellipsoid(f'Slate_accent_cushion_{side}',(x,1.215,-.795),(.37,.155,.245),'pillow',.50,64,40,taper=.26)\n# A compact lumbar cushion in muted steel blue to layer the pillows.\nsuperellipsoid('Lumbar_cushion',(0,1.245,-.57),(.46,.135,.155),'throw',.53,64,36,taper=.30)\n\n# Structural headboard shell and seven individual, vertically channeled upholstered pads.\n# The channels have eased shoulders, a softly bowed face, and visible shadow seams.\nsuperellipsoid('Headboard_back_shell',(0,1.185,-1.575),(1.29,.925,.115),'charcoal2',.50,88,52)\nsuperellipsoid('Headboard_top_rail',(0,2.035,-1.475),(1.245,.085,.125),'charcoal',.55,72,36)\nfor i in range(7):\n    x=(i-3)*.354\n    superellipsoid(f'Headboard_channel_{i+1:02d}',(x,1.275,-1.435),(.174,.735,.112),'charcoal',.50,64,44,taper=.18)\n    # Delicate stitched inset down the centre of every padded section.\n    curve_tube(f'Headboard_channel_stitch_{i+1:02d}',[(x,0.72+j*.043,-1.326+.006*math.cos(j*.18)) for j in range(29)],.00115,'charcoal2')\n# Side wings roll forward just a little, framing the upholstered centre panel.\nfor side in (-1,1):\n    superellipsoid(f'Headboard_side_bolster_{side}',(side*1.205,1.255,-1.415),(.105,.77,.135),'charcoal2',.50,56,36)\n# Rear support brackets are hidden in normal view but add a credible connection to the frame.\nfor side in (-1,1):\n    boxsoft(f'Headboard_mount_{side}',(side*.92,.74,-1.53),(.105,.48,.13),'underside',.40)\n"

NEW_GEOMETRY = "# ----------------------- PILLOWS AND HEADBOARD -----------------------\n# Two substantial sleep pillows plus smaller slate accent cushions, each independently modeled.\nfor side in (-1,1):\n    x=side*.49\n    superellipsoid(f'Sleep_pillow_{side}',(x,1.095,-1.115),(.52,.175,.335),'sheet',.48,72,44,taper=.34)\n    # Fine seam along the pillow's visible front crown.\n    curve_tube(f'Sleep_pillow_piping_{side}',[(x+u,1.19,-1.405+.025*math.cos(u*2.2)) for u in np.linspace(-.43,.43,28)],.0024,'pillow')\nfor side in (-1,1):\n    x=side*.285\n    superellipsoid(f'Slate_accent_cushion_{side}',(x,1.215,-.795),(.37,.155,.245),'pillow',.50,64,40,taper=.26)\n# A compact lumbar cushion in muted steel blue to layer the pillows.\nsuperellipsoid('Lumbar_cushion',(0,1.245,-.57),(.46,.135,.155),'throw',.53,64,36,taper=.30)\n\n# One continuous upholstered headboard pad: no diamonds, buttons or segmented cushions.
# The broad face has a gentle crown and eased perimeter, like a single padded panel.
superellipsoid(
    'Headboard_single_upholstered_panel',
    (0,1.205,-1.445),
    (1.165,.725,.115),'charcoal',.58,112,64,taper=.08
)

# Deep backing keeps the pad visually integrated with the original headboard shell.
superellipsoid(
    'Headboard_outer_shell',
    (0,1.185,-1.575),
    (1.325,.925,.13),'charcoal2',.50,96,56
)
boxsoft(
    'Headboard_rear_backing',
    (0,1.185,-1.455),
    (2.52,1.70,.12),'charcoal2',.42
)

# Two nested, softly eased frame lips form a real recessed light channel.
# The LED emitter sits between the lips, behind their front edges: frontal views
# can see the glow, while a low bed-level view sees only reflected spill.
for side in (-1,1):
    superellipsoid(
        f'Headboard_outer_lip_side_{side}',
        (side*1.365,1.185,-1.205),
        (.045,.835,.105),'charcoal2',.48,72,44
    )
    superellipsoid(
        f'Headboard_inner_lip_side_{side}',
        (side*1.155,1.185,-1.205),
        (.04,.745,.105),'charcoal',.52,72,44
    )

for name,y in (('top',2.115),('bottom',.245)):
    superellipsoid(
        f'Headboard_outer_lip_edge_{name}',
        (0,y,-1.205),
        (1.365,.045,.105),'charcoal2',.48,88,40
    )
for name,y in (('top',1.845),('bottom',.525)):
    superellipsoid(
        f'Headboard_inner_lip_edge_{name}',
        (0,y,-1.205),
        (1.155,.04,.105),'charcoal',.52,88,40
    )

# A subtle upholstered seam follows the single panel's perimeter, not its face.
curve_tube(
    'Headboard_panel_perimeter_seam',
    [(-1.105,.59,-1.322),(-1.105,1.82,-1.322),
     (1.105,1.82,-1.322),(1.105,.59,-1.322),
     (-1.105,.59,-1.322)],
    .0022,'charcoal2',sides=8
)

# The native renderer supplies the colour-changing recessed glow in the channel.
# No exposed LED tape, bulbs, diamond tufting or decorative buttons are modeled.
# Original mounting brackets and every non-headboard bed component stay unchanged.
\nfor side in (-1,1):\n    boxsoft(f'Headboard_mount_{side}',(side*.92,.74,-1.53),(.105,.48,.13),'underside',.40)"

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
    new_design = "'design':'Single smooth charcoal upholstered headboard panel with gently eased edges and a deeply recessed, hidden-angle RGB perimeter glow channel; original bed frame, mattress and bedding retained',"
    if source.count(old_design) != 1:
        raise RuntimeError("Bed metadata design marker changed; update the design patch.")
    source = source.replace(old_design, new_design, 1)
    return source
