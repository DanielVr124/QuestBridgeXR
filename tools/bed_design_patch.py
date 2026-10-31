"""Targeted geometry update for the editable upholstered bed generator.

The original generator is gzip-packed to keep the repository compact. This patch
is deliberately isolated in plain Python so the bed design remains reviewable.
"""
from __future__ import annotations

OLD_GEOMETRY = "# ----------------------- PILLOWS AND HEADBOARD -----------------------\n# Two substantial sleep pillows plus smaller slate accent cushions, each independently modeled.\nfor side in (-1,1):\n    x=side*.49\n    superellipsoid(f'Sleep_pillow_{side}',(x,1.095,-1.115),(.52,.175,.335),'sheet',.48,72,44,taper=.34)\n    # Fine seam along the pillow's visible front crown.\n    curve_tube(f'Sleep_pillow_piping_{side}',[(x+u,1.19,-1.405+.025*math.cos(u*2.2)) for u in np.linspace(-.43,.43,28)],.0024,'pillow')\nfor side in (-1,1):\n    x=side*.285\n    superellipsoid(f'Slate_accent_cushion_{side}',(x,1.215,-.795),(.37,.155,.245),'pillow',.50,64,40,taper=.26)\n# A compact lumbar cushion in muted steel blue to layer the pillows.\nsuperellipsoid('Lumbar_cushion',(0,1.245,-.57),(.46,.135,.155),'throw',.53,64,36,taper=.30)\n\n# Structural headboard shell and seven individual, vertically channeled upholstered pads.\n# The channels have eased shoulders, a softly bowed face, and visible shadow seams.\nsuperellipsoid('Headboard_back_shell',(0,1.185,-1.575),(1.29,.925,.115),'charcoal2',.50,88,52)\nsuperellipsoid('Headboard_top_rail',(0,2.035,-1.475),(1.245,.085,.125),'charcoal',.55,72,36)\nfor i in range(7):\n    x=(i-3)*.354\n    superellipsoid(f'Headboard_channel_{i+1:02d}',(x,1.275,-1.435),(.174,.735,.112),'charcoal',.50,64,44,taper=.18)\n    # Delicate stitched inset down the centre of every padded section.\n    curve_tube(f'Headboard_channel_stitch_{i+1:02d}',[(x,0.72+j*.043,-1.326+.006*math.cos(j*.18)) for j in range(29)],.00115,'charcoal2')\n# Side wings roll forward just a little, framing the upholstered centre panel.\nfor side in (-1,1):\n    superellipsoid(f'Headboard_side_bolster_{side}',(side*1.205,1.255,-1.415),(.105,.77,.135),'charcoal2',.50,56,36)\n# Rear support brackets are hidden in normal view but add a credible connection to the frame.\nfor side in (-1,1):\n    boxsoft(f'Headboard_mount_{side}',(side*.92,.74,-1.53),(.105,.48,.13),'underside',.40)\n"

NEW_GEOMETRY = "# ----------------------- PILLOWS AND HEADBOARD -----------------------\n# Two substantial sleep pillows plus smaller slate accent cushions, each independently modeled.\nfor side in (-1,1):\n    x=side*.49\n    superellipsoid(f'Sleep_pillow_{side}',(x,1.095,-1.115),(.52,.175,.335),'sheet',.48,72,44,taper=.34)\n    # Fine seam along the pillow's visible front crown.\n    curve_tube(f'Sleep_pillow_piping_{side}',[(x+u,1.19,-1.405+.025*math.cos(u*2.2)) for u in np.linspace(-.43,.43,28)],.0024,'pillow')\nfor side in (-1,1):\n    x=side*.285\n    superellipsoid(f'Slate_accent_cushion_{side}',(x,1.215,-.795),(.37,.155,.245),'pillow',.50,64,40,taper=.26)\n# A compact lumbar cushion in muted steel blue to layer the pillows.\nsuperellipsoid('Lumbar_cushion',(0,1.245,-.57),(.46,.135,.155),'throw',.53,64,36,taper=.30)\n\n# ----------------------- SINGLE-PANEL LED HEADBOARD ONLY -----------------------\n# Keep the original headboard footprint and frame. Replace every tuft/diamond\n# with one uninterrupted, softly bevelled upholstered slab. Its front face\n# reaches the inside edge of the LED diffuser so there is no empty border gap.\nsuperellipsoid(\n    'Headboard_outer_shell',(0,1.185,-1.575),\n    (1.29,.925,.115),'charcoal2',.56,112,72\n)\nboxsoft(\n    'Headboard_single_upholstered_panel',\n    (0,1.175,-1.255),(2.15,1.38,.22),\n    'charcoal',.12\n)\nsuperellipsoid(\n    'Headboard_top_rail',(0,2.035,-1.475),\n    (1.245,.085,.125),'charcoal2',.55,72,36\n)\n\n# Satin-silver outer trim remains visible around the panel.\nheadboard_silver_outline=[\n    (-1.145,.405,-1.132),(-1.145,1.945,-1.132),\n    (1.145,1.945,-1.132),(1.145,.405,-1.132),\n    (-1.145,.405,-1.132)\n]\ncurve_tube('Headboard_silver_grey_perimeter',headboard_silver_outline,.020,'sheet')\n\n# A continuous pale silicone LED diffuser is mounted directly on the panel\n# edge. Its inner edge touches the upholstered slab; its outer edge sits just\n# inside the silver trim. The node name keeps the in-app RGB remote control.\nheadboard_led_outline=[\n    (-1.085,.475,-1.140),(-1.085,1.875,-1.140),\n    (1.085,1.875,-1.140),(1.085,.475,-1.140),\n    (-1.085,.475,-1.140)\n]\ncurve_tube('Headboard_blue_violet_LED_reveal',headboard_led_outline,.012,'led')\n\n# Retain the original padded side wings and hidden mounting brackets.\nfor side in (-1,1):\n    superellipsoid(\n        f'Headboard_side_bolster_{side}',\n        (side*1.205,1.255,-1.415),(.105,.77,.135),\n        'charcoal2',.50,56,36\n    )\nfor side in (-1,1):\n    boxsoft(f'Headboard_mount_{side}',(side*.92,.74,-1.53),(.105,.48,.13),'underside',.40)\n"

def patch_bed_geometry(source: str) -> str:
    if source.count(OLD_GEOMETRY) != 1:
        raise RuntimeError(
            "Bed geometry patch did not find exactly one original pillow/headboard block; "
            "refusing to generate an unexpected model."
        )
    source = source.replace(OLD_GEOMETRY, NEW_GEOMETRY, 1)

    # The native GLB importer identifies controllable parts by node name. Keep
    # the LED diffuser separate from the eight fabric material batches, and
    # give it an emissive glTF material so it is visible in Blender renders too.
    material_marker = "    materials[key]=mat\\n\\nscene=trimesh.Scene(base_frame='BedRoot')"
    material_replacement = """    materials[key]=mat

# Dedicated emissive diffuser; isolated so the RGB remote can target its node.
materials['led']=PBRMaterial(
    name='Headboard LED diffuser',
    baseColorFactor=[180,205,255,255],
    emissiveFactor=[0.22,0.38,1.0],
    metallicFactor=0.0,
    roughnessFactor=0.34,
    doubleSided=True
)

scene=trimesh.Scene(base_frame='BedRoot')"""
    if source.count(material_marker) != 1:
        raise RuntimeError("Bed generator material block changed; cannot isolate LED material.")
    source = source.replace(material_marker, material_replacement, 1)

    runtime_marker = "runtime_scene.add_geometry(merged,geom_name=f'Bed_{key}',node_name=f'Bed_{key}')"
    runtime_replacement = """runtime_name='Bed_Headboard_LED_reveal' if key=='led' else f'Bed_{key}'
        runtime_scene.add_geometry(merged,geom_name=runtime_name,node_name=runtime_name)"""
    if source.count(runtime_marker) != 1:
        raise RuntimeError("Bed generator runtime merge block changed; cannot name LED node.")
    source = source.replace(runtime_marker, runtime_replacement, 1)
    old_design = "'design':'Charcoal channel-upholstered headboard, padded upholstered rails, silver cotton bedding, graphite duvet, midnight ribbed throw',"
    new_design = "'design':'Single smooth charcoal upholstered headboard panel with an integrated controllable LED diffuser and satin-silver trim; original bed frame, mattress and bedding retained',"
    if source.count(old_design) != 1:
        raise RuntimeError("Bed metadata design marker changed; update the design patch.")
    source = source.replace(old_design, new_design, 1)
    return source
