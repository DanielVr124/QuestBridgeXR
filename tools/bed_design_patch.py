"""Targeted geometry update for the editable upholstered bed generator.

The original generator is gzip-packed to keep the repository compact. This patch
is deliberately isolated in plain Python so the bed design remains reviewable.
"""
from __future__ import annotations

OLD_GEOMETRY = "# ----------------------- PILLOWS AND HEADBOARD -----------------------\n# Two substantial sleep pillows plus smaller slate accent cushions, each independently modeled.\nfor side in (-1,1):\n    x=side*.49\n    superellipsoid(f'Sleep_pillow_{side}',(x,1.095,-1.115),(.52,.175,.335),'sheet',.48,72,44,taper=.34)\n    # Fine seam along the pillow's visible front crown.\n    curve_tube(f'Sleep_pillow_piping_{side}',[(x+u,1.19,-1.405+.025*math.cos(u*2.2)) for u in np.linspace(-.43,.43,28)],.0024,'pillow')\nfor side in (-1,1):\n    x=side*.285\n    superellipsoid(f'Slate_accent_cushion_{side}',(x,1.215,-.795),(.37,.155,.245),'pillow',.50,64,40,taper=.26)\n# A compact lumbar cushion in muted steel blue to layer the pillows.\nsuperellipsoid('Lumbar_cushion',(0,1.245,-.57),(.46,.135,.155),'throw',.53,64,36,taper=.30)\n\n# Structural headboard shell and seven individual, vertically channeled upholstered pads.\n# The channels have eased shoulders, a softly bowed face, and visible shadow seams.\nsuperellipsoid('Headboard_back_shell',(0,1.185,-1.575),(1.29,.925,.115),'charcoal2',.50,88,52)\nsuperellipsoid('Headboard_top_rail',(0,2.035,-1.475),(1.245,.085,.125),'charcoal',.55,72,36)\nfor i in range(7):\n    x=(i-3)*.354\n    superellipsoid(f'Headboard_channel_{i+1:02d}',(x,1.275,-1.435),(.174,.735,.112),'charcoal',.50,64,44,taper=.18)\n    # Delicate stitched inset down the centre of every padded section.\n    curve_tube(f'Headboard_channel_stitch_{i+1:02d}',[(x,0.72+j*.043,-1.326+.006*math.cos(j*.18)) for j in range(29)],.00115,'charcoal2')\n# Side wings roll forward just a little, framing the upholstered centre panel.\nfor side in (-1,1):\n    superellipsoid(f'Headboard_side_bolster_{side}',(side*1.205,1.255,-1.415),(.105,.77,.135),'charcoal2',.50,56,36)\n# Rear support brackets are hidden in normal view but add a credible connection to the frame.\nfor side in (-1,1):\n    boxsoft(f'Headboard_mount_{side}',(side*.92,.74,-1.53),(.105,.48,.13),'underside',.40)\n"

NEW_GEOMETRY = """
# Keep the original bed platform, rails, legs, foot end, mattress, fitted sheet,
# duvet and throw untouched; this patch replaces only the pillows/headboard.
# ----------------------- PILLOWS AND HEADBOARD -----------------------
# ----------------------- PILLOWS -----------------------
# Preserve the two main sleep-pillow centres and rebuild the accent layout.
# Replace only the primitive sleep-pillow ellipsoids with tailored, softly
# squared cotton shells: gently puffed crowns, rounded corners, and real gussets.
PILLOW_HALF_WIDTH = .46
PILLOW_HALF_DEPTH = .31
PILLOW_EDGE_TOP = .025
PILLOW_EDGE_BOTTOM = -.105
PILLOW_CROWN_HEIGHT = .145
PILLOW_CORNER_EXPONENT = 3.5
PILLOW_RING_SEGMENTS = 112
PILLOW_RING_ROWS = 20
PILLOW_REAR_TILT = math.radians(8.0)

def pillow_outline(angle, scale=1.0):
    power = 2.0 / PILLOW_CORNER_EXPONENT
    cosine = math.cos(angle)
    sine = math.sin(angle)
    return (
        PILLOW_HALF_WIDTH * scale * math.copysign(abs(cosine) ** power, cosine),
        PILLOW_HALF_DEPTH * scale * math.copysign(abs(sine) ** power, sine),
    )

def pillow_crown_height(x, z, side):
    # A restrained dome keeps the pillow full without making it look inflated.
    gauge = (
        (abs(x) / PILLOW_HALF_WIDTH) ** PILLOW_CORNER_EXPONENT
        + (abs(z) / PILLOW_HALF_DEPTH) ** PILLOW_CORNER_EXPONENT
    )
    radius = min(1.0, max(0.0, gauge ** (1.0 / PILLOW_CORNER_EXPONENT)))
    crown = PILLOW_CROWN_HEIGHT * max(0.0, 1.0 - radius * radius) ** .72

    # Fine, low-amplitude cloth relaxation fades into the sewn perimeter.
    crease_envelope = radius ** 2.2 * math.sqrt(max(0.0, 1.0 - radius))
    fine_relaxation = (
        .0016 * math.sin(22.0 * x + side * .7)
        + .0011 * math.sin(19.0 * z - 7.0 * x + side * .35)
    ) * crease_envelope
    return PILLOW_EDGE_TOP + crown + fine_relaxation

for side in (-1, 1):
    # These centres match the previous bed layout exactly.
    pillow_center = np.array([side*.49, 1.095, -1.115], dtype=float)
    pillow_rotation = trimesh.transformations.rotation_matrix(
        PILLOW_REAR_TILT, (1.0, 0.0, 0.0)
    )

    def pillow_world(x, y, z):
        local = np.array([x, y, z], dtype=float)
        return pillow_rotation[:3, :3] @ local + pillow_center

    # Soft domed top, built from concentric rounded-rectangle rings.
    top_vertices = [pillow_world(0.0, pillow_crown_height(0.0, 0.0, side), 0.0)]
    top_uv = [(.5, .5)]
    for ring in range(1, PILLOW_RING_ROWS + 1):
        radius = ring / PILLOW_RING_ROWS
        for segment in range(PILLOW_RING_SEGMENTS):
            angle = 2.0 * math.pi * segment / PILLOW_RING_SEGMENTS
            ox, oz = pillow_outline(angle, radius)
            top_vertices.append(pillow_world(ox, pillow_crown_height(ox, oz, side), oz))
            top_uv.append((
                .5 + ox / (2.0 * PILLOW_HALF_WIDTH),
                .5 + oz / (2.0 * PILLOW_HALF_DEPTH),
            ))

    top_faces = []
    first_ring = 1
    for segment in range(PILLOW_RING_SEGMENTS):
        nxt = (segment + 1) % PILLOW_RING_SEGMENTS
        top_faces.append((0, first_ring + nxt, first_ring + segment))
    for ring in range(PILLOW_RING_ROWS - 1):
        inner = 1 + ring * PILLOW_RING_SEGMENTS
        outer = inner + PILLOW_RING_SEGMENTS
        for segment in range(PILLOW_RING_SEGMENTS):
            nxt = (segment + 1) % PILLOW_RING_SEGMENTS
            a, b = inner + segment, inner + nxt
            c, d = outer + nxt, outer + segment
            top_faces.extend(((a, b, c), (a, c, d)))
    add_mesh(f'Sleep_pillow_top_{side}', top_vertices, top_faces, top_uv, 'sheet')

    # A full-depth side gusset has a slight outward belly between its seams.
    # Tiny perimeter slack adds fabric softness without scalloping the edge.
    gusset_vertices = []
    gusset_uv = []
    gusset_rows = 14
    for row in range(gusset_rows + 1):
        t = row / gusset_rows
        y = PILLOW_EDGE_BOTTOM + (PILLOW_EDGE_TOP - PILLOW_EDGE_BOTTOM) * t
        belly = .012 * math.sin(math.pi * t)
        for segment in range(PILLOW_RING_SEGMENTS):
            angle = 2.0 * math.pi * segment / PILLOW_RING_SEGMENTS
            ox, oz = pillow_outline(angle, 1.0 + belly)
            gusset_vertices.append(pillow_world(ox, y, oz))
            gusset_uv.append((segment / PILLOW_RING_SEGMENTS, t))

    gusset_faces = []
    for row in range(gusset_rows):
        inner = row * PILLOW_RING_SEGMENTS
        outer = inner + PILLOW_RING_SEGMENTS
        for segment in range(PILLOW_RING_SEGMENTS):
            nxt = (segment + 1) % PILLOW_RING_SEGMENTS
            a, b = inner + segment, inner + nxt
            c, d = outer + nxt, outer + segment
            gusset_faces.extend(((a, b, c), (a, c, d)))
    add_mesh(
        f'Sleep_pillow_box_gusset_{side}',
        gusset_vertices, gusset_faces, gusset_uv, 'pillow'
    )

    # The underside closes the shell and preserves believable thickness from
    # low angles; it is separate so the source stays easy to edit in Blender.
    bottom_vertices = [pillow_world(0.0, PILLOW_EDGE_BOTTOM - .012, 0.0)]
    bottom_uv = [(.5, .5)]
    for ring in range(1, PILLOW_RING_ROWS + 1):
        radius = ring / PILLOW_RING_ROWS
        for segment in range(PILLOW_RING_SEGMENTS):
            angle = 2.0 * math.pi * segment / PILLOW_RING_SEGMENTS
            ox, oz = pillow_outline(angle, radius)
            y = PILLOW_EDGE_BOTTOM - .012 * (1.0 - radius * radius)
            bottom_vertices.append(pillow_world(ox, y, oz))
            bottom_uv.append((
                .5 + ox / (2.0 * PILLOW_HALF_WIDTH),
                .5 + oz / (2.0 * PILLOW_HALF_DEPTH),
            ))

    bottom_faces = []
    for segment in range(PILLOW_RING_SEGMENTS):
        nxt = (segment + 1) % PILLOW_RING_SEGMENTS
        bottom_faces.append((0, 1 + segment, 1 + nxt))
    for ring in range(PILLOW_RING_ROWS - 1):
        inner = 1 + ring * PILLOW_RING_SEGMENTS
        outer = inner + PILLOW_RING_SEGMENTS
        for segment in range(PILLOW_RING_SEGMENTS):
            nxt = (segment + 1) % PILLOW_RING_SEGMENTS
            a, b = inner + segment, inner + nxt
            c, d = outer + nxt, outer + segment
            bottom_faces.extend(((a, c, b), (a, d, c)))
    add_mesh(
        f'Sleep_pillow_underside_{side}',
        bottom_vertices, bottom_faces, bottom_uv, 'sheet'
    )

    def pillow_top_world(x, z, lift=.003):
        return pillow_world(x, pillow_crown_height(x, z, side) + lift, z)

    # Narrow welt at the crown edge and a smaller lower seam make the gusset
    # construction readable without giving the pillow an over-trimmed look.
    top_welt = []
    lower_gusset_seam = []
    for segment in range(144):
        angle = 2.0 * math.pi * segment / 144
        ox, oz = pillow_outline(angle, .992)
        top_welt.append(pillow_top_world(ox, oz, .0015))
        lower_gusset_seam.append(
            pillow_world(ox, PILLOW_EDGE_BOTTOM + .014, oz)
        )
    curve_tube(f'Sleep_pillow_top_welt_{side}', top_welt, .0036, 'linen')
    curve_tube(
        f'Sleep_pillow_lower_gusset_seam_{side}',
        lower_gusset_seam, .0024, 'pillow'
    )

    # One very subtle eased crease near each corner suggests a soft filling
    # settling inside the casing; it does not turn the pillow into a quilt.
    for corner_x in (-1, 1):
        for corner_z in (-1, 1):
            points = []
            for step in range(12):
                t = step / 11.0
                x = corner_x * (.345 + .075 * t)
                z = corner_z * (.195 + .035 * t)
                points.append(pillow_top_world(x, z, .0018))
            curve_tube(
                f'Sleep_pillow_corner_fold_{side}_{corner_x}_{corner_z}',
                points, .0015, 'pillow', sides=6
            )

# Two compact graphite accents sit just forward of the sleeping pillows.
# Their footprints are deliberately separated, so they read as two cushions
# instead of intersecting with each other or merging into a central bolster.
for side in (-1, 1):
    x = side * .235
    superellipsoid(
        f'Slate_accent_cushion_{side}', (x, 1.14, -.66),
        (.205, .105, .145), 'pillow', .58, 80, 52, taper=.18
    )
    curve_tube(
        f'Slate_accent_piping_{side}',
        [(x-.16, 1.17, -.79), (x, 1.205, -.805), (x+.16, 1.17, -.79)],
        .0015, 'throw', sides=6
    )

# Keep a small centre lumbar accent as a separate front layer; it no longer
# intersects the main sleeping pillows or the two graphite accent cushions.
superellipsoid(
    'Lumbar_cushion', (0, 1.03, -.34),
    (.18, .065, .085), 'throw', .60, 72, 48, taper=.16
)
curve_tube(
    'Lumbar_cushion_seam',
    [(-.13, 1.065, -.414), (0, 1.085, -.42), (.13, 1.065, -.414)],
    .0012, 'pillow', sides=6
)

# ----------------------- BRAMBLEY-STYLE HEADBOARD -----------------------
# Keep the existing bed platform, rails, legs, foot end, mattress and bedding.
# The headboard is rebuilt as the Brambley reference: silver crystal-tufted
# faux-leather insert, beveled mirror frame, embossed outer bands and hidden LED channel.
superellipsoid(
    'Headboard_structural_backing',(0,1.185,-1.575),
    (1.29,.925,.115),'headboard_leather',.48,112,72
)

# Silver alligator-embossed outer bands. The small raised scales are a restrained
# surface treatment, not separate vertical cushions or ribs.
for side in (-1,1):
    superellipsoid(
        f'Headboard_alligator_side_band_{side}',
        (side*1.205,1.185,-1.445),
        (.078,.79,.045),'headboard_emboss_vertical',.52,88,48,taper=.04
    )
for name,y in (('top',2.005),('bottom',.365)):
    superellipsoid(
        f'Headboard_alligator_{name}_band',
        (0,y,-1.445),
        (1.18,.078,.045),'headboard_emboss_horizontal',.52,96,40,taper=.04
    )

# The faux-alligator grain is mapped into each silver band material.
# This keeps the embossed surface continuous instead of adding chunky raised tiles.

# Beveled silver frame with a dark shadow line so the inner mirror edge reads
# as a separate polished bevel instead of a flat painted border.
for name,y in (('top',1.925),('bottom',.445)):
    superellipsoid(
        f'Headboard_mirror_bevel_{name}',
        (0,y,-1.382),
        (1.165,.042,.030),'headboard_chrome',.66,96,32
    )
for side in (-1,1):
    superellipsoid(
        f'Headboard_mirror_bevel_side_{side}',
        (side*1.165,1.185,-1.382),
        (.042,.74,.030),'headboard_chrome',.66,72,32
    )
# Fine inner dark reveal follows the silver bevel around the upholstered insert.
curve_tube(
    'Headboard_inner_shadow_reveal',
    [(-1.105,.525,-1.335),(-1.105,1.845,-1.335),
     (1.105,1.845,-1.335),(1.105,.525,-1.335)],
    .008,'charcoal2',sides=8
)

# One rounded, closed perimeter raceway follows the complete outer headboard edge.
# Repeating the first point closes the tube into one continuous mesh with no corner gaps.
perimeter_points = []
led_corner_radius = .070
led_corners = (
    (1.165,1.885,0,90),
    (-1.165,1.885,90,180),
    (-1.165,.475,180,270),
    (1.165,.475,270,360),
)
for cx,cy,start_angle,end_angle in led_corners:
    for step in range(13):
        angle=math.radians(start_angle+(end_angle-start_angle)*step/12)
        perimeter_points.append((
            cx+led_corner_radius*math.cos(angle),
            cy+led_corner_radius*math.sin(angle)
        ))
perimeter_points.append(perimeter_points[0])

channel_points=[(x,y,-1.355) for x,y in perimeter_points]
strip_points=[(x,y,-1.345) for x,y in perimeter_points]
curve_tube(
    'Headboard_connected_outer_LED_channel',
    channel_points,.018,'headboard_led_channel',sides=12
)
curve_tube(
    'Headboard_connected_outer_LED_strip',
    strip_points,.009,'headboard_led_diffuser',sides=12
)

# Single silver bi-cast-vinyl foundation under individually puffed diamond cells.
superellipsoid(
    'Headboard_crystal_tufted_silver_insert',
    (0,1.185,-1.415),
    (1.075,.685,.135),'headboard_leather',.22,160,96,taper=.015
)

# Five alternating crystal-button rows form a continuous 5-4-5-4-5 tuft grid.
tuft_rows = (
    (-.82,-.41,0,.41,.82),
    (-.615,-.205,.205,.615),
    (-.82,-.41,0,.41,.82),
    (-.615,-.205,.205,.615),
    (-.82,-.41,0,.41,.82),
)
tuft_y = (1.70,1.43,1.16,.89,.62)

# Build each diamond as a smooth, raised mesh patch with a soft crown and
# an eased edge that sinks into the recessed seam around the cell.
def _add_brambley_quilt_diamond(name,cx,cy,rx,ry,z_edge=-1.285,puff=.085,rings=18,segments=72):
    verts = [(cx,cy,z_edge+puff)]
    uv = [(.5,.5)]
    for ring in range(1,rings+1):
        radius = ring/rings
        z = z_edge + puff*((1.0-radius*radius)**2.0)
        for k in range(segments):
            t = 4.0*k/segments
            edge = int(t)
            f = t-edge
            if edge == 0:
                ux,vy = f,1.0-f
            elif edge == 1:
                ux,vy = 1.0-f,-f
            elif edge == 2:
                ux,vy = -f,-(1.0-f)
            else:
                ux,vy = -(1.0-f),f
            verts.append((cx+rx*radius*ux,cy+ry*radius*vy,z))
            uv.append(((ux+1.0)*.5,(vy+1.0)*.5))

    faces = []
    first = 1
    for k in range(segments):
        faces.append((0,first+k,first+(k+1)%segments))
    for ring in range(1,rings):
        inner = 1+(ring-1)*segments
        outer = 1+ring*segments
        for k in range(segments):
            kn = (k+1)%segments
            a0 = inner+k
            b0 = inner+kn
            c0 = outer+kn
            d0 = outer+k
            faces.extend(((a0,b0,c0),(a0,c0,d0)))
    add_mesh(name,verts,faces,uv,'headboard_leather')

# Ten raised diamond upholstery cells. Their narrow gaps meet at the crystal
# buttons, so the tufting is built into the surface rather than drawn on top.
for band in (0,2):
    top_y = tuft_y[band]
    bottom_y = tuft_y[band+2]
    center_y = (top_y+bottom_y)*.5
    radius_y = (top_y-bottom_y)*.5-.012
    for col,cx in enumerate(tuft_rows[band]):
        _add_brambley_quilt_diamond(
            f'Headboard_quilt_diamond_{band}_{col}',
            cx,center_y,.198,radius_y
        )

# Fine inset seams follow the diagonals between adjacent tuft points.
for row in range(4):
    upper = tuft_rows[row]
    lower = tuft_rows[row+1]
    for col,x in enumerate(upper):
        for next_col,nx in enumerate(lower):
            if abs(x-nx) <= .206:
                curve_tube(
                    f'Headboard_tuft_seam_{row}_{col}_{next_col}',
                    [(x,tuft_y[row],-1.289),
                     ((x+nx)*.5,(tuft_y[row]+tuft_y[row+1])*.5,-1.292),
                     (nx,tuft_y[row+1],-1.289)],
                    .0018,'headboard_seam',sides=8
                )

# Bright chrome sockets and small polished crystal cabochons at all 23 junctions.
for row,(xs,y) in enumerate(zip(tuft_rows,tuft_y)):
    for col,x in enumerate(xs):
        superellipsoid(
            f'Headboard_crystal_button_setting_{row}_{col}',
            (x,y,-1.190),(.026,.026,.013),'headboard_chrome',1.08,36,24
        )
        superellipsoid(
            f'Headboard_crystal_tuft_button_{row}_{col}',
            (x,y,-1.166),(.020,.020,.017),'headboard_crystal',1.18,48,32
        )

# Original mounting brackets are retained exactly.
for side in (-1,1):
    boxsoft(f'Headboard_mount_{side}',(side*.92,.74,-1.53),(.105,.48,.13),'underside',.40)
"""

IMAGE_IMPORT_OLD = "from PIL import Image, ImageFilter\n"
IMAGE_IMPORT_NEW = "from PIL import Image, ImageFilter, ImageDraw\n"
HEADBOARD_MATERIALS = r"""
# ----------------------- BRAMBLEY HEADBOARD PBR MATERIALS -----------------------
# Pearl-silver bi-cast vinyl, directional alligator embossing, polished mirror
# bevels, crystal hardware, and the recessed perimeter LED raceway.
def _make_headboard_alligator_texture(name, cell_width, cell_height):
    size=512
    image=Image.new('RGB',(size,size),(165,172,184))
    draw=ImageDraw.Draw(image)
    rows=int(math.ceil(size/cell_height))+2
    cols=int(math.ceil(size/cell_width))+2
    for row in range(-1,rows):
        row_offset=cell_width//2 if row%2 else 0
        for col in range(-1,cols):
            cx=col*cell_width+cell_width//2+row_offset
            cy=row*cell_height+cell_height//2
            left=int(cx-cell_width*.43)
            top=int(cy-cell_height*.43)
            right=int(cx+cell_width*.43)
            bottom=int(cy+cell_height*.43)
            radius=max(2,int(min(cell_width,cell_height)*.28))
            draw.rounded_rectangle(
                (left,top,right,bottom),radius=radius,
                fill=(177,184,197),outline=(139,148,163),width=1
            )
            box=(left+1,top+1,right-1,bottom-1)
            draw.arc(box,190,350,fill=(211,218,229),width=1)
            draw.arc(box,10,170,fill=(145,154,169),width=1)
    image=image.filter(ImageFilter.GaussianBlur(.38))
    image.save(TEXTURES/f'{name}.png',optimize=True)
    return image

_headboard_alligator_h=_make_headboard_alligator_texture(
    'headboard_alligator_horizontal',14,82
)
_headboard_alligator_v=_make_headboard_alligator_texture(
    'headboard_alligator_vertical',82,27
)
materials['headboard_leather']=PBRMaterial(
    name='Pearl silver bi-cast vinyl',
    baseColorFactor=[255,255,255,255],metallicFactor=.06,roughnessFactor=.54,
    baseColorTexture=textures['linen'],doubleSided=True
)
materials['headboard_emboss_horizontal']=PBRMaterial(
    name='Silver embossed faux-alligator horizontal panels',
    baseColorFactor=[255,255,255,255],metallicFactor=.18,roughnessFactor=.43,
    baseColorTexture=_headboard_alligator_h,doubleSided=True
)
materials['headboard_emboss_vertical']=PBRMaterial(
    name='Silver embossed faux-alligator vertical panels',
    baseColorFactor=[255,255,255,255],metallicFactor=.18,roughnessFactor=.43,
    baseColorTexture=_headboard_alligator_v,doubleSided=True
)
materials['headboard_chrome']=PBRMaterial(
    name='Polished beveled mirror chrome',
    baseColorFactor=[210,219,232,255],metallicFactor=.90,roughnessFactor=.16,
    doubleSided=True
)
materials['headboard_seam']=PBRMaterial(
    name='Soft recessed silver-vinyl tuft seam',
    baseColorFactor=[92,99,112,255],metallicFactor=.02,roughnessFactor=.82,
    doubleSided=True
)
materials['headboard_led_channel']=PBRMaterial(
    name='Dark recessed outer LED raceway',
    baseColorFactor=[18,24,42,255],metallicFactor=.16,roughnessFactor=.48,
    doubleSided=True
)
materials['headboard_led_diffuser']=PBRMaterial(
    name='Headboard blue-violet LED diffuser',
    baseColorFactor=[58,102,236,255],metallicFactor=.03,roughnessFactor=.24,
    emissiveFactor=[.035,.042,.060],doubleSided=True
)
materials['headboard_crystal']=PBRMaterial(
    name='Clear polished crystal tuft hardware',
    baseColorFactor=[242,248,255,255],metallicFactor=.04,roughnessFactor=.07,
    emissiveFactor=[.035,.045,.065],doubleSided=True
)
"""

def patch_bed_geometry(source: str) -> str:
    if source.count(OLD_GEOMETRY) != 1:
        raise RuntimeError(
            "Bed geometry patch did not find exactly one original pillow/headboard block; "
            "refusing to generate an unexpected model."
        )
    source = source.replace(OLD_GEOMETRY, NEW_GEOMETRY, 1)

    if source.count(IMAGE_IMPORT_OLD) != 1:
        raise RuntimeError("Bed generator PIL import marker changed; cannot add headboard materials.")
    source = source.replace(IMAGE_IMPORT_OLD, IMAGE_IMPORT_NEW, 1)

    material_marker = "    materials[key]=mat\n"
    if source.count(material_marker) != 1:
        raise RuntimeError("Bed generator material map marker changed; cannot add headboard materials.")
    source = source.replace(material_marker, material_marker + HEADBOARD_MATERIALS, 1)

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
    new_design = "'design':'Original bed platform and bedding retained; Brambley-style pearl-silver faux-leather with ten raised diamond-quilt cells, 23 crystal tuft points, mirror-chrome bevels, embossed silver alligator bands and recessed perimeter RGB backlight',"
    if source.count(old_design) != 1:
        raise RuntimeError("Bed metadata design marker changed; update the design patch.")
    source = source.replace(old_design, new_design, 1)
    return source
