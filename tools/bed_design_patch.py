"""Targeted geometry update for the editable upholstered bed generator.

The original generator is gzip-packed to keep the repository compact. This patch
is deliberately isolated in plain Python so the bed design remains reviewable.
"""
from __future__ import annotations

OLD_GEOMETRY = "# ----------------------- PILLOWS AND HEADBOARD -----------------------\n# Two substantial sleep pillows plus smaller slate accent cushions, each independently modeled.\nfor side in (-1,1):\n    x=side*.49\n    superellipsoid(f'Sleep_pillow_{side}',(x,1.095,-1.115),(.52,.175,.335),'sheet',.48,72,44,taper=.34)\n    # Fine seam along the pillow's visible front crown.\n    curve_tube(f'Sleep_pillow_piping_{side}',[(x+u,1.19,-1.405+.025*math.cos(u*2.2)) for u in np.linspace(-.43,.43,28)],.0024,'pillow')\nfor side in (-1,1):\n    x=side*.285\n    superellipsoid(f'Slate_accent_cushion_{side}',(x,1.215,-.795),(.37,.155,.245),'pillow',.50,64,40,taper=.26)\n# A compact lumbar cushion in muted steel blue to layer the pillows.\nsuperellipsoid('Lumbar_cushion',(0,1.245,-.57),(.46,.135,.155),'throw',.53,64,36,taper=.30)\n\n# Structural headboard shell and seven individual, vertically channeled upholstered pads.\n# The channels have eased shoulders, a softly bowed face, and visible shadow seams.\nsuperellipsoid('Headboard_back_shell',(0,1.185,-1.575),(1.29,.925,.115),'charcoal2',.50,88,52)\nsuperellipsoid('Headboard_top_rail',(0,2.035,-1.475),(1.245,.085,.125),'charcoal',.55,72,36)\nfor i in range(7):\n    x=(i-3)*.354\n    superellipsoid(f'Headboard_channel_{i+1:02d}',(x,1.275,-1.435),(.174,.735,.112),'charcoal',.50,64,44,taper=.18)\n    # Delicate stitched inset down the centre of every padded section.\n    curve_tube(f'Headboard_channel_stitch_{i+1:02d}',[(x,0.72+j*.043,-1.326+.006*math.cos(j*.18)) for j in range(29)],.00115,'charcoal2')\n# Side wings roll forward just a little, framing the upholstered centre panel.\nfor side in (-1,1):\n    superellipsoid(f'Headboard_side_bolster_{side}',(side*1.205,1.255,-1.415),(.105,.77,.135),'charcoal2',.50,56,36)\n# Rear support brackets are hidden in normal view but add a credible connection to the frame.\nfor side in (-1,1):\n    boxsoft(f'Headboard_mount_{side}',(side*.92,.74,-1.53),(.105,.48,.13),'underside',.40)\n"

NEW_GEOMETRY = """# ----------------------- PILLOWS AND HEADBOARD -----------------------
# Two genuinely boxy side-sleeper pillows.  Each has a deep gusset, softened
# corners, a tailored perimeter welt and a fine diamond-quilted cotton top.
PILLOW_TILT = math.radians(15.0)
for side in (-1,1):
    center=np.array([side*.515,1.105,-1.075],dtype=float)
    half_width,half_height,half_depth=.535,.205,.325
    pillow=superellipsoid(
        f'Sleep_pillow_{side}',tuple(center),
        (half_width,half_height,half_depth),'sheet',.58,112,72,taper=.10
    )
    pillow_rotation=trimesh.transformations.rotation_matrix(
        PILLOW_TILT,(1.0,0.0,0.0),point=center
    )
    pillow.apply_transform(pillow_rotation)

    def pillow_surface_point(u,v,lift=.004):
        fullness=max(0.0,1.0-(abs(u)/half_width)**4-(abs(v)/half_depth)**4)
        crown=half_height*.91*math.pow(fullness,.43)
        local=np.array([center[0]+u,center[1]+crown+lift,center[2]+v,1.0])
        return (pillow_rotation @ local)[:3]

    # A full-height side gusset is called out by a stitched lower seam and
    # a second welt line just under the crown.  Both follow rounded box corners.
    gusset_points=[]
    crown_points=[]
    for angle in np.linspace(0.0,2.0*math.pi,112,endpoint=False):
        ca,sa=math.cos(angle),math.sin(angle)
        u=half_width*.965*math.copysign(abs(ca)**.52,ca)
        v=half_depth*.965*math.copysign(abs(sa)**.52,sa)
        lower=np.array([center[0]+u,center[1]-.075,center[2]+v,1.0])
        gusset_points.append((pillow_rotation @ lower)[:3])
        crown_points.append(pillow_surface_point(u*.975,v*.975,.001))
    gusset=curve_tube(f'Sleep_pillow_gusset_seam_{side}',gusset_points,.0045,'pillow')
    gusset.apply_transform(pillow_rotation)
    crown_welt=curve_tube(f'Sleep_pillow_crown_welt_{side}',crown_points,.0032,'pillow')

    # Crossed stitch rows follow the inflated top instead of floating above it.
    for family in (-1,1):
        for row,offset in enumerate(np.linspace(-.235,.235,9)):
            points=[]
            for u in np.linspace(-.47,.47,80):
                v=family*(u*half_depth/half_width)+offset
                if (abs(u)/half_width)**4+(abs(v)/half_depth)**4 <= .88:
                    points.append(pillow_surface_point(u,v,.006))
            if len(points)>3:
                stitch=curve_tube(
                    f'Sleep_pillow_quilt_{side}_{family}_{row:02d}',
                    points,.0018,'pillow'
                )
    # A few gently eased corner darts make the case read as sewn fabric.
    for corner_x in (-1,1):
        for corner_z in (-1,1):
            dart_local=[
                (center[0]+corner_x*.42,center[1]+.112,center[2]+corner_z*.245),
                (center[0]+corner_x*.465,center[1]+.128,center[2]+corner_z*.285),
                (center[0]+corner_x*.485,center[1]+.112,center[2]+corner_z*.302)
            ]
            dart=[(pillow_rotation @ np.array([*p,1.0]))[:3] for p in dart_local]
            dart_mesh=curve_tube(f'Sleep_pillow_corner_dart_{side}_{corner_x}_{corner_z}',dart,.0020,'pillow')

# Soft accent pillows stay behind the main pair, with the same sewn corners
# and quieter graphite fabric so the white side-sleeper pillows remain dominant.
for side in (-1,1):
    accent_center=np.array([side*.30,1.20,-1.245],dtype=float)
    accent=superellipsoid(
        f'Graphite_accent_pillow_{side}',tuple(accent_center),
        (.34,.16,.22),'pillow',.58,80,52,taper=.12
    )
    accent_rotation=trimesh.transformations.rotation_matrix(
        math.radians(side*7.0),(1.0,0.0,0.0),point=accent_center
    )
    accent.apply_transform(accent_rotation)
    accent_welt_points=[]
    for angle in np.linspace(0.0,2.0*math.pi,88,endpoint=False):
        ca,sa=math.cos(angle),math.sin(angle)
        p=np.array([
            accent_center[0]+.325*math.copysign(abs(ca)**.55,ca),
            accent_center[1]-.018,
            accent_center[2]+.205*math.copysign(abs(sa)**.55,sa),1.0
        ])
        accent_welt_points.append((accent_rotation @ p)[:3])
    accent_welt=curve_tube(f'Graphite_accent_welt_{side}',accent_welt_points,.003,'charcoal2')

# ----------------------- DIAMOND-TUFTED HEADBOARD -----------------------
# Wide black upholstered shell and deeply padded front face.
superellipsoid(
    'Headboard_outer_shell',(0,1.27,-1.625),
    (1.32,.91,.14),'charcoal2',.56,112,72
)
superellipsoid(
    'Headboard_tufted_backing',(0,1.27,-1.425),
    (1.235,.79,.17),'charcoal',.56,112,72
)
# Broad rolled wings give the headboard the squared, furniture-like silhouette.
for side in (-1,1):
    wing_center=np.array([side*1.105,1.27,-1.385],dtype=float)
    wing=superellipsoid(
        f'Headboard_wingback_{side}',tuple(wing_center),
        (.17,.825,.245),'charcoal2',.55,88,56
    )
    wing.apply_transform(trimesh.transformations.rotation_matrix(
        math.radians(side*12.0),(0.0,1.0,0.0),point=wing_center
    ))

# Each lozenge is a separate softly crowned cushion.  Tiny bright pull-buttons
# at the lattice intersections create real tuft depth rather than a flat decal.
for row in range(3):
    y=.91+row*.36
    row_shift=.215 if row%2 else 0.0
    for column in range(5):
        x=(column-2)*.43+row_shift
        diamond_center=np.array([x,y,-1.175],dtype=float)
        diamond=superellipsoid(
            f'Headboard_diamond_tuft_{row+1:02d}_{column+1:02d}',
            tuple(diamond_center),(.195,.145,.067),
            'charcoal',.48,64,44,taper=.08
        )
        diamond.apply_transform(trimesh.transformations.rotation_matrix(
            math.radians(45.0),(0.0,0.0,1.0),point=diamond_center
        ))

# Crystal-like graphite/silver buttons are seated in the recessed tuft points.
for row in range(4):
    y=.73+row*.36
    for column in range(6):
        x=-1.075+column*.43+(row%2)*.215
        superellipsoid(
            f'Headboard_tuft_button_{row+1:02d}_{column+1:02d}',
            (x,y,-1.103),(.026,.026,.018),'sheet',.52,32,20
        )

# A silver-grey raised frame and an inset blue-violet LED reveal follow the
# perimeter.  These are actual modeled trims, not a flat image texture.
headboard_silver_outline=[
    (-1.17,.64,-1.105),(-1.17,1.90,-1.105),
    (1.17,1.90,-1.105),(1.17,.64,-1.105),
    (-1.17,.64,-1.105)
]
curve_tube('Headboard_silver_grey_perimeter',headboard_silver_outline,.022,'sheet')
headboard_led_outline=[
    (-1.105,.70,-1.115),(-1.105,1.84,-1.115),
    (1.105,1.84,-1.115),(1.105,.70,-1.115),
    (-1.105,.70,-1.115)
]
curve_tube('Headboard_blue_violet_LED_reveal',headboard_led_outline,.008,'throw')

# ----------------------- MATCHING DIAMOND FOOTBOARD -----------------------
# The footboard receives the same black padded panel, diamond cushions,
# inset silver edge and subtle under-edge LED detail as the reference bed.
superellipsoid(
    'Footboard_outer_shell',(0,.565,1.615),
    (1.275,.425,.155),'charcoal2',.56,96,60
)
superellipsoid(
    'Footboard_tufted_backing',(0,.565,1.795),
    (1.19,.345,.135),'charcoal',.56,96,56
)
for row in range(2):
    y=.435+row*.26
    row_shift=.205 if row%2 else 0.0
    for column in range(5):
        x=(column-2)*.41+row_shift
        center=np.array([x,y,1.905],dtype=float)
        pad=superellipsoid(
            f'Footboard_diamond_tuft_{row+1:02d}_{column+1:02d}',
            tuple(center),(.185,.115,.058),'charcoal',.48,56,36,taper=.08
        )
        pad.apply_transform(trimesh.transformations.rotation_matrix(
            math.radians(45.0),(0.0,0.0,1.0),point=center
        ))
for row in range(3):
    y=.305+row*.26
    for column in range(6):
        x=-1.025+column*.41+(row%2)*.205
        superellipsoid(
            f'Footboard_tuft_button_{row+1:02d}_{column+1:02d}',
            (x,y,1.969),(.022,.022,.016),'sheet',.52,28,18
        )
footboard_silver_outline=[
    (-1.105,.255,1.966),(-1.105,.875,1.966),
    (1.105,.875,1.966),(1.105,.255,1.966),
    (-1.105,.255,1.966)
]
curve_tube('Footboard_silver_grey_perimeter',footboard_silver_outline,.018,'sheet')
footboard_led_outline=[
    (-1.045,.295,1.976),(-1.045,.835,1.976),
    (1.045,.835,1.976),(1.045,.295,1.976),
    (-1.045,.295,1.976)
]
curve_tube('Footboard_blue_violet_LED_reveal',footboard_led_outline,.006,'throw')

# Hidden mounting brackets connect the headboard to the platform frame.
for side in (-1,1):
    boxsoft(f'Headboard_mount_{side}',(side*.92,.74,-1.53),(.105,.48,.13),'underside',.40)
"""

def patch_bed_geometry(source: str) -> str:
    if source.count(OLD_GEOMETRY) != 1:
        raise RuntimeError(
            "Bed geometry patch did not find exactly one original pillow/headboard block; "
            "refusing to generate an unexpected model."
        )
    source = source.replace(OLD_GEOMETRY, NEW_GEOMETRY, 1)
    old_design = "'design':'Charcoal channel-upholstered headboard, padded upholstered rails, silver cotton bedding, graphite duvet, midnight ribbed throw',"
    new_design = "'design':'Wide sculpted charcoal wingback headboard, two propped hotel pillows, silver cotton bedding, graphite duvet, midnight ribbed throw',"
    if source.count(old_design) != 1:
        raise RuntimeError("Bed metadata design marker changed; update the design patch.")
    return source.replace(old_design, new_design, 1)
