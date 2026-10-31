"""Targeted geometry update for the editable upholstered bed generator.

The original generator is gzip-packed to keep the repository compact. This patch
is deliberately isolated in plain Python so the bed design remains reviewable.
"""
from __future__ import annotations

OLD_GEOMETRY = "# ----------------------- PILLOWS AND HEADBOARD -----------------------\n# Two substantial sleep pillows plus smaller slate accent cushions, each independently modeled.\nfor side in (-1,1):\n    x=side*.49\n    superellipsoid(f'Sleep_pillow_{side}',(x,1.095,-1.115),(.52,.175,.335),'sheet',.48,72,44,taper=.34)\n    # Fine seam along the pillow's visible front crown.\n    curve_tube(f'Sleep_pillow_piping_{side}',[(x+u,1.19,-1.405+.025*math.cos(u*2.2)) for u in np.linspace(-.43,.43,28)],.0024,'pillow')\nfor side in (-1,1):\n    x=side*.285\n    superellipsoid(f'Slate_accent_cushion_{side}',(x,1.215,-.795),(.37,.155,.245),'pillow',.50,64,40,taper=.26)\n# A compact lumbar cushion in muted steel blue to layer the pillows.\nsuperellipsoid('Lumbar_cushion',(0,1.245,-.57),(.46,.135,.155),'throw',.53,64,36,taper=.30)\n\n# Structural headboard shell and seven individual, vertically channeled upholstered pads.\n# The channels have eased shoulders, a softly bowed face, and visible shadow seams.\nsuperellipsoid('Headboard_back_shell',(0,1.185,-1.575),(1.29,.925,.115),'charcoal2',.50,88,52)\nsuperellipsoid('Headboard_top_rail',(0,2.035,-1.475),(1.245,.085,.125),'charcoal',.55,72,36)\nfor i in range(7):\n    x=(i-3)*.354\n    superellipsoid(f'Headboard_channel_{i+1:02d}',(x,1.275,-1.435),(.174,.735,.112),'charcoal',.50,64,44,taper=.18)\n    # Delicate stitched inset down the centre of every padded section.\n    curve_tube(f'Headboard_channel_stitch_{i+1:02d}',[(x,0.72+j*.043,-1.326+.006*math.cos(j*.18)) for j in range(29)],.00115,'charcoal2')\n# Side wings roll forward just a little, framing the upholstered centre panel.\nfor side in (-1,1):\n    superellipsoid(f'Headboard_side_bolster_{side}',(side*1.205,1.255,-1.415),(.105,.77,.135),'charcoal2',.50,56,36)\n# Rear support brackets are hidden in normal view but add a credible connection to the frame.\nfor side in (-1,1):\n    boxsoft(f'Headboard_mount_{side}',(side*.92,.74,-1.53),(.105,.48,.13),'underside',.40)\n"

NEW_GEOMETRY = """# ----------------------- PILLOWS AND HEADBOARD -----------------------
# Box-gusset side-sleeper pillows: a true rounded-rectangle footprint,
# a separately meshed domed cotton crown, and a full-depth sewn side wall.
# The crown is puffed in the centre and gently relaxed at the sewn perimeter.
PILLOW_TILT = math.radians(10.0)
PILLOW_HALF_WIDTH = .535
PILLOW_HALF_DEPTH = .325
PILLOW_EDGE_TOP = .028
PILLOW_EDGE_BOTTOM = -.128
PILLOW_CROWN = .168
PILLOW_CORNER_EXPONENT = 4.2
PILLOW_RING_SEGMENTS = 112
PILLOW_RING_ROWS = 20

def pillow_outline(u, scale=1.0):
    # Superellipse perimeter gives visible pillow corners without a hard box.
    power = 2.0 / PILLOW_CORNER_EXPONENT
    return (
        PILLOW_HALF_WIDTH * scale * math.copysign(abs(math.cos(u))**power, math.cos(u)),
        PILLOW_HALF_DEPTH * scale * math.copysign(abs(math.sin(u))**power, math.sin(u))
    )

def pillow_crown_y(x, z):
    gauge = (
        (abs(x) / PILLOW_HALF_WIDTH)**PILLOW_CORNER_EXPONENT
        + (abs(z) / PILLOW_HALF_DEPTH)**PILLOW_CORNER_EXPONENT
    )
    radius = min(1.0, max(0.0, gauge**(1.0/PILLOW_CORNER_EXPONENT)))
    falloff = max(0.0, 1.0-radius*radius)**.72
    # Small cloth relaxations break up the mathematically smooth dome, but fade
    # to zero at the seam so the crown welds cleanly into the gusset wall.
    wrinkle = .0032 * math.sin(18.0*x+4.5*z) * math.sin(16.0*z-3.2*x)
    wrinkle *= radius**3 * math.sqrt(max(0.0,1.0-radius))
    return PILLOW_EDGE_TOP + PILLOW_CROWN*falloff + wrinkle

for side in (-1,1):
    pillow_center=np.array([side*.515,1.08,-1.06],dtype=float)
    pillow_rotation=trimesh.transformations.rotation_matrix(
        PILLOW_TILT,(1.0,0.0,0.0)
    )
    def pillow_world(x,y,z):
        local=np.array([x,y,z],dtype=float)
        return pillow_rotation[:3,:3] @ local + pillow_center

    # Domed top shell, formed from concentric rounded-rectangle rings.
    # The dense radial topology supports the soft cloth crown and visible quilting.
    top_vertices=[pillow_world(0.0,pillow_crown_y(0.0,0.0),0.0)]
    top_uv=[(.5,.5)]
    for ring in range(1,PILLOW_RING_ROWS+1):
        radius=ring/PILLOW_RING_ROWS
        for segment in range(PILLOW_RING_SEGMENTS):
            angle=2.0*math.pi*segment/PILLOW_RING_SEGMENTS
            ox,oz=pillow_outline(angle,radius)
            top_vertices.append(pillow_world(ox,pillow_crown_y(ox,oz),oz))
            top_uv.append((.5+ox/(2*PILLOW_HALF_WIDTH),.5+oz/(2*PILLOW_HALF_DEPTH)))
    top_faces=[]
    first_ring=1
    for segment in range(PILLOW_RING_SEGMENTS):
        nxt=(segment+1)%PILLOW_RING_SEGMENTS
        top_faces.append((0,first_ring+nxt,first_ring+segment))
    for ring in range(PILLOW_RING_ROWS-1):
        inner=1+ring*PILLOW_RING_SEGMENTS
        outer=inner+PILLOW_RING_SEGMENTS
        for segment in range(PILLOW_RING_SEGMENTS):
            nxt=(segment+1)%PILLOW_RING_SEGMENTS
            a=inner+segment; b=inner+nxt
            c=outer+nxt; d=outer+segment
            top_faces.extend([(a,b,c),(a,c,d)])
    add_mesh(f'Sleep_pillow_top_{side}',top_vertices,top_faces,top_uv,'sheet')

    # Deep continuous gusset sidewall. Its middle bows out slightly like a
    # filled pillow case, while the top/bottom seams stay neatly tucked in.
    gusset_vertices=[]; gusset_uv=[]
    gusset_rows=14
    for row in range(gusset_rows+1):
        t=row/gusset_rows
        y=PILLOW_EDGE_BOTTOM+(PILLOW_EDGE_TOP-PILLOW_EDGE_BOTTOM)*t
        bulge=.022*math.sin(math.pi*t)
        for segment in range(PILLOW_RING_SEGMENTS):
            angle=2.0*math.pi*segment/PILLOW_RING_SEGMENTS
            ox,oz=pillow_outline(angle,1.0+bulge)
            gusset_vertices.append(pillow_world(ox,y,oz))
            gusset_uv.append((segment/PILLOW_RING_SEGMENTS,t))
    gusset_faces=[]
    for row in range(gusset_rows):
        inner=row*PILLOW_RING_SEGMENTS
        outer=inner+PILLOW_RING_SEGMENTS
        for segment in range(PILLOW_RING_SEGMENTS):
            nxt=(segment+1)%PILLOW_RING_SEGMENTS
            a=inner+segment; b=inner+nxt
            c=outer+nxt; d=outer+segment
            gusset_faces.extend([(a,b,c),(a,c,d)])
    add_mesh(f'Sleep_pillow_box_gusset_{side}',gusset_vertices,gusset_faces,gusset_uv,'pillow')

    # Slightly crowned underside closes the pillow shell and keeps its thickness
    # believable when viewed from a low VR camera.
    bottom_vertices=[pillow_world(0.0,PILLOW_EDGE_BOTTOM-.018,0.0)]
    bottom_uv=[(.5,.5)]
    for ring in range(1,PILLOW_RING_ROWS+1):
        radius=ring/PILLOW_RING_ROWS
        for segment in range(PILLOW_RING_SEGMENTS):
            angle=2.0*math.pi*segment/PILLOW_RING_SEGMENTS
            ox,oz=pillow_outline(angle,radius)
            y=PILLOW_EDGE_BOTTOM-.018*(1.0-radius*radius)
            bottom_vertices.append(pillow_world(ox,y,oz))
            bottom_uv.append((.5+ox/(2*PILLOW_HALF_WIDTH),.5+oz/(2*PILLOW_HALF_DEPTH)))
    bottom_faces=[]
    for segment in range(PILLOW_RING_SEGMENTS):
        nxt=(segment+1)%PILLOW_RING_SEGMENTS
        bottom_faces.append((0,1+segment,1+nxt))
    for ring in range(PILLOW_RING_ROWS-1):
        inner=1+ring*PILLOW_RING_SEGMENTS
        outer=inner+PILLOW_RING_SEGMENTS
        for segment in range(PILLOW_RING_SEGMENTS):
            nxt=(segment+1)%PILLOW_RING_SEGMENTS
            a=inner+segment; b=inner+nxt
            c=outer+nxt; d=outer+segment
            bottom_faces.extend([(a,c,b),(a,d,c)])
    add_mesh(f'Sleep_pillow_underside_{side}',bottom_vertices,bottom_faces,bottom_uv,'sheet')

    def pillow_top_world(x,z,lift=.004):
        return pillow_world(x,pillow_crown_y(x,z)+lift,z)

    # Raised welt at the top edge and a second stitched seam at the lower edge
    # make the deep gusset obvious from the front/side camera.
    top_welt=[]
    lower_gusset_seam=[]
    for segment in range(144):
        angle=2.0*math.pi*segment/144
        ox,oz=pillow_outline(angle,.985)
        top_welt.append(pillow_top_world(ox,oz,.002))
        bx,bz=pillow_outline(angle,.985)
        lower_gusset_seam.append(pillow_world(bx,PILLOW_EDGE_BOTTOM+.018,bz))
    curve_tube(f'Sleep_pillow_top_welt_{side}',top_welt,.0042,'linen')
    curve_tube(f'Sleep_pillow_lower_gusset_seam_{side}',lower_gusset_seam,.0034,'linen')

    # Diamond quilting follows the actual puffed crown, so it stays attached to
    # the mesh rather than floating above or disappearing beneath the fabric.
    for family in (-1,1):
        for row,offset in enumerate(np.linspace(-.235,.235,7)):
            points=[]
            for x in np.linspace(-.48,.48,96):
                z=family*.58*x+offset
                gauge=(abs(x)/PILLOW_HALF_WIDTH)**PILLOW_CORNER_EXPONENT + (abs(z)/PILLOW_HALF_DEPTH)**PILLOW_CORNER_EXPONENT
                if gauge<=.84:
                    points.append(pillow_top_world(x,z,.003))
            if len(points)>3:
                curve_tube(
                    f'Sleep_pillow_diamond_quilt_{side}_{family}_{row:02d}',
                    points,.0018,'linen'
                )

    # Short eased darts at the four corners reinforce the tailored, sewn case.
    for corner_x in (-1,1):
        for corner_z in (-1,1):
            points=[]
            for step in range(12):
                t=step/11.0
                x=corner_x*(.37+.105*t)
                z=corner_z*(.235+.065*t)
                points.append(pillow_top_world(x,z,.0035))
            curve_tube(
                f'Sleep_pillow_corner_dart_{side}_{corner_x}_{corner_z}',
                points,.0021,'linen'
            )

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
    new_design = "'design':'Black diamond-tufted LED headboard and matching footboard, silver-grey perimeter trim, quilted gusset pillows, graphite duvet and midnight-blue throw',"
    if source.count(old_design) != 1:
        raise RuntimeError("Bed metadata design marker changed; update the design patch.")
    return source.replace(old_design, new_design, 1)
