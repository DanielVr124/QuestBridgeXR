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
# Keep the existing pillow arrangement and placement; this task is a headboard-only correction.
for side in (-1,1):
    x=side*.49
    superellipsoid(
        f'Sleep_pillow_{side}',(x,1.095,-1.115),
        (.535,.165,.335),'sheet',.54,88,56,taper=.24
    )
    curve_tube(
        f'Sleep_pillow_piping_{side}',
        [(x+u,1.188,-1.405+.022*math.cos(u*2.2)) for u in np.linspace(-.44,.44,36)],
        .0022,'pillow'
    )
    for flank in (-1,1):
        curve_tube(
            f'Sleep_pillow_corner_fold_{side}_{flank}',
            [(x+flank*.43,1.145,-1.407),
             (x+flank*.475,1.165,-1.385),
             (x+flank*.49,1.176,-1.352)],
            .00125,'pillow',sides=6
        )

for side in (-1,1):
    x=side*.285
    superellipsoid(
        f'Slate_accent_cushion_{side}',(x,1.215,-.795),
        (.355,.145,.235),'pillow',.56,80,52,taper=.22
    )
    curve_tube(
        f'Slate_accent_piping_{side}',
        [(x-.285,1.245,-.985),(x,1.278,-1.015),(x+.285,1.245,-.985)],
        .0017,'throw',sides=6
    )

superellipsoid(
    'Lumbar_cushion',(0,1.245,-.57),
    (.445,.125,.145),'throw',.56,80,48,taper=.20
)
curve_tube(
    'Lumbar_cushion_seam',
    [(-.36,1.292,-.638),(0,1.315,-.642),(.36,1.292,-.638)],
    .0015,'pillow',sides=6
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

# LED channel is recessed behind the mirror frame at the existing compositor
# perimeter coordinates. The diffuser itself remains hidden from a reclined view.
for name,y in (('top',1.955),('bottom',.405)):
    boxsoft(
        f'Headboard_recessed_LED_channel_{name}',
        (0,y,-1.485),(2.47,.025,.032),'charcoal2',.22
    )
for side in (-1,1):
    boxsoft(
        f'Headboard_recessed_LED_channel_side_{side}',
        (side*1.235,1.185,-1.485),(.025,1.55,.032),'charcoal2',.22
    )

# One full-width softly crowned silver faux-leather insert.
superellipsoid(
    'Headboard_crystal_tufted_silver_insert',
    (0,1.185,-1.415),
    (1.055,.685,.128),'headboard_leather',.50,144,88,taper=.025
)

# Crystal-button tufting: four even rows of five clear, faceted-looking studs.
# Each stud has a small shadowed setting under a bright raised crystal.
tuft_x = (-.82,-.41,0,.41,.82)
tuft_y = (1.705,1.355,1.015,.665)
for row,y in enumerate(tuft_y):
    for col,x in enumerate(tuft_x):
        superellipsoid(
            f'Headboard_crystal_button_setting_{row}_{col}',
            (x,y,-1.286),(.036,.036,.012),'headboard_chrome',.72,32,20
        )
        superellipsoid(
            f'Headboard_crystal_tuft_button_{row}_{col}',
            (x,y,-1.266),(.024,.024,.015),'headboard_crystal',.92,40,28
        )

# Soft diagonal seams create the diamond-tufted upholstery between the buttons.
for row in range(3):
    for col in range(5):
        x=tuft_x[col]
        if col < 4:
            nx=tuft_x[col+1]
            curve_tube(
                f'Headboard_tuft_seam_down_right_{row}_{col}',
                [(x,tuft_y[row],-1.291),
                 ((x+nx)*.5, (tuft_y[row]+tuft_y[row+1])*.5,-1.296),
                 (nx,tuft_y[row+1],-1.291)],
                .0022,'charcoal2',sides=6
            )
        if col > 0:
            nx=tuft_x[col-1]
            curve_tube(
                f'Headboard_tuft_seam_down_left_{row}_{col}',
                [(x,tuft_y[row],-1.291),
                 ((x+nx)*.5, (tuft_y[row]+tuft_y[row+1])*.5,-1.296),
                 (nx,tuft_y[row+1],-1.291)],
                .0022,'charcoal2',sides=6
            )

# Original mounting brackets are retained exactly.
for side in (-1,1):
    boxsoft(f'Headboard_mount_{side}',(side*.92,.74,-1.53),(.105,.48,.13),'underside',.40)
"""

IMAGE_IMPORT_OLD = "from PIL import Image, ImageFilter\n"
IMAGE_IMPORT_NEW = "from PIL import Image, ImageFilter, ImageDraw\n"
HEADBOARD_MATERIALS = "\n# ----------------------- BRAMBLEY HEADBOARD PBR MATERIALS -----------------------\n# Pearl-silver bi-cast vinyl, directional alligator embossing, polished mirror\n# bevels, and clear polished crystal hardware are dedicated PBR materials.\ndef _make_headboard_alligator_texture(name, cell_width, cell_height):\n    size=512\n    image=Image.new('RGB',(size,size),(165,172,184))\n    draw=ImageDraw.Draw(image)\n    rows=int(math.ceil(size/cell_height))+2\n    cols=int(math.ceil(size/cell_width))+2\n    for row in range(-1,rows):\n        row_offset=cell_width//2 if row%2 else 0\n        for col in range(-1,cols):\n            cx=col*cell_width+cell_width//2+row_offset\n            cy=row*cell_height+cell_height//2\n            left=int(cx-cell_width*.43)\n            top=int(cy-cell_height*.43)\n            right=int(cx+cell_width*.43)\n            bottom=int(cy+cell_height*.43)\n            radius=max(2,int(min(cell_width,cell_height)*.28))\n            draw.rounded_rectangle(\n                (left,top,right,bottom),radius=radius,\n                fill=(177,184,197),outline=(139,148,163),width=1\n            )\n            box=(left+1,top+1,right-1,bottom-1)\n            draw.arc(box,190,350,fill=(211,218,229),width=1)\n            draw.arc(box,10,170,fill=(145,154,169),width=1)\n    image=image.filter(ImageFilter.GaussianBlur(.38))\n    image.save(TEXTURES/f'{name}.png',optimize=True)\n    return image\n\n_headboard_alligator_h=_make_headboard_alligator_texture(\n    'headboard_alligator_horizontal',14,82\n)\n_headboard_alligator_v=_make_headboard_alligator_texture(\n    'headboard_alligator_vertical',82,27\n)\nmaterials['headboard_leather']=PBRMaterial(\n    name='Pearl silver bi-cast vinyl',\n    baseColorFactor=[255,255,255,255],metallicFactor=.06,roughnessFactor=.54,\n    baseColorTexture=textures['linen'],doubleSided=True\n)\nmaterials['headboard_emboss_horizontal']=PBRMaterial(\n    name='Silver embossed faux-alligator horizontal panels',\n    baseColorFactor=[255,255,255,255],metallicFactor=.18,roughnessFactor=.43,\n    baseColorTexture=_headboard_alligator_h,doubleSided=True\n)\nmaterials['headboard_emboss_vertical']=PBRMaterial(\n    name='Silver embossed faux-alligator vertical panels',\n    baseColorFactor=[255,255,255,255],metallicFactor=.18,roughnessFactor=.43,\n    baseColorTexture=_headboard_alligator_v,doubleSided=True\n)\nmaterials['headboard_chrome']=PBRMaterial(\n    name='Polished beveled mirror chrome',\n    baseColorFactor=[210,219,232,255],metallicFactor=.90,roughnessFactor=.16,\n    doubleSided=True\n)\nmaterials['headboard_crystal']=PBRMaterial(\n    name='Clear polished crystal tuft hardware',\n    baseColorFactor=[242,248,255,255],metallicFactor=.04,roughnessFactor=.07,\n    emissiveFactor=[.035,.045,.065],doubleSided=True\n)\n"

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
    new_design = "'design':'Original bed platform and bedding retained; Brambley-style pearl-silver bi-cast vinyl diamond-tufted headboard insert, polished mirror chrome bevels, direction-mapped faux-alligator silver panels, clear crystal buttons and recessed perimeter RGB backlight',"
    if source.count(old_design) != 1:
        raise RuntimeError("Bed metadata design marker changed; update the design patch.")
    source = source.replace(old_design, new_design, 1)
    return source
