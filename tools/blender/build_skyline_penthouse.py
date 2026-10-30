#!/usr/bin/env python3
"""Build the QuestBridge XR neon-noir penthouse in Blender.

Produces an editable .blend, a GLB interchange model, the compact QBX runtime mesh,
and three PNG renders from this exact scene. Room footprint matches the Quest
renderer medium preset: 8.4m x 10.8m x 4.4m.
"""
import bpy, math, os, sys, struct, random
from mathutils import Vector
from math import sin, cos, pi

random.seed(421)
ARGS = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
OUT = "/tmp/penthouse-build"
if "--output-dir" in ARGS: OUT = ARGS[ARGS.index("--output-dir") + 1]
os.makedirs(OUT, exist_ok=True)
RENDER_DIR = os.path.join(OUT, "renders")
os.makedirs(RENDER_DIR, exist_ok=True)
W, D, H = 8.4, 10.8, 4.4

SCENE_COL = bpy.data.collections.new("QBX_RuntimeGeometry")
bpy.context.scene.collection.children.link(SCENE_COL)
LIGHT_COL = bpy.data.collections.new("Lighting")
bpy.context.scene.collection.children.link(LIGHT_COL)
CAM_COL = bpy.data.collections.new("Cameras")
bpy.context.scene.collection.children.link(CAM_COL)

def move_to(obj, collection=SCENE_COL):
    for c in list(obj.users_collection): c.objects.unlink(obj)
    collection.objects.link(obj)
    return obj

def bevel(obj, width=.025, segments=3):
    if width > 0:
        mod=obj.modifiers.new("Soft machined edges",'BEVEL'); mod.width=width; mod.segments=segments
        obj.modifiers.new("Weighted corner normals",'WEIGHTED_NORMAL')
    return obj

def mat(name, color, rough=.5, metal=0., shader_id=2, noise_scale=None, noise_strength=0.,
        emission=None, emission_strength=0., transmission=0.):
    m=bpy.data.materials.new(name); m.diffuse_color=(*color,1); m.use_nodes=True
    m["qbx_shader_id"]=int(shader_id)
    n=m.node_tree.nodes; l=m.node_tree.links; bs=n.get("Principled BSDF")
    bs.inputs["Base Color"].default_value=(*color,1)
    bs.inputs["Roughness"].default_value=rough; bs.inputs["Metallic"].default_value=metal
    if "Transmission Weight" in bs.inputs: bs.inputs["Transmission Weight"].default_value=transmission
    elif "Transmission" in bs.inputs: bs.inputs["Transmission"].default_value=transmission
    if emission:
        if "Emission Color" in bs.inputs: bs.inputs["Emission Color"].default_value=(*emission,1)
        elif "Emission" in bs.inputs: bs.inputs["Emission"].default_value=(*emission,1)
        if "Emission Strength" in bs.inputs: bs.inputs["Emission Strength"].default_value=emission_strength
    if noise_scale:
        tex=n.new("ShaderNodeTexNoise"); tex.inputs["Scale"].default_value=noise_scale
        tex.inputs["Detail"].default_value=5.; tex.inputs["Roughness"].default_value=.72
        bump=n.new("ShaderNodeBump"); bump.inputs["Strength"].default_value=noise_strength
        bump.inputs["Distance"].default_value=.018
        l.new(tex.outputs["Fac"],bump.inputs["Height"]); l.new(bump.outputs["Normal"],bs.inputs["Normal"])
    return m

M={}
M["stone"]=mat("Obsidian marble | honed black",(.026,.034,.060),.20,.19,8,5,.16)
M["stone_light"]=mat("Smoked quartz stone",(.075,.090,.132),.24,.12,8,4,.11)
M["wall"]=mat("Graphite limewash plaster",(.044,.052,.080),.88,0,2,75,.11)
M["felt"]=mat("Midnight acoustic felt",(.055,.064,.103),.95,0,3,210,.17)
M["fabric"]=mat("Midnight blue woven upholstery",(.045,.066,.120),.93,0,3,260,.20)
M["fabric_light"]=mat("Slate woven upholstery",(.105,.125,.190),.92,0,3,310,.17)
M["duvet"]=mat("Charcoal brushed cotton duvet",(.085,.096,.142),.91,0,3,340,.12)
M["leather"]=mat("Black aniline leather",(.025,.033,.050),.35,.02,5,85,.07)
M["wood"]=mat("Smoked walnut veneer",(.085,.050,.032),.34,.03,1,8,.11)
M["wood_dark"]=mat("Black stained oak",(.025,.028,.040),.31,.06,1,13,.08)
M["metal"]=mat("Black anodized aluminium",(.055,.070,.090),.27,.82,4,150,.04)
M["chrome"]=mat("Brushed titanium",(.36,.40,.47),.25,.88,4,180,.035)
M["brass"]=mat("Champagne brushed brass",(.38,.25,.12),.29,.78,4,210,.025)
M["glass"]=mat("Smoked architectural glass",(.012,.022,.043),.13,.55,9,None,0,transmission=.18)
M["screen"]=mat("OLED black glass",(.004,.009,.020),.12,.22,-1)
M["screen_glow"]=mat("Monitor cyan violet artwork",(.025,.12,.36),.20,.05,7,None,0,(.045,.20,.82),2.8)
M["blue"]=mat("Electric blue LED diffuser",(.04,.35,1.),.24,0,7,None,0,(.015,.22,1.),4.)
M["violet"]=mat("Violet LED diffuser",(.38,.08,1.),.22,0,7,None,0,(.32,.045,1.),3.5)
M["pink"]=mat("Magenta LED diffuser",(1.,.035,.38),.22,0,7,None,0,(1.,.015,.28),3.)
M["warm"]=mat("Warm opal practical light",(1.,.56,.26),.24,0,7,None,0,(1.,.43,.14),2.3)
M["plant"]=mat("Deep emerald foliage",(.025,.16,.095),.68,0,2,42,.14)
M["plant2"]=mat("Satin leaf highlights",(.055,.25,.14),.52,0,2,55,.09)
M["rug"]=mat("Plush graphite wool rug",(.065,.075,.102),.98,0,3,500,.25)
M["ceramic"]=mat("Matte charcoal ceramic",(.025,.032,.047),.32,.05,5,38,.04)
M["white"]=mat("Warm porcelain",(.46,.43,.38),.26,.03,5,95,.025)
M["city"]=mat("City tower concrete",(.018,.028,.052),.8,.08,2,12,.04)
M["city_cyan"]=mat("Tower windows cyan",(.06,.35,.72),.25,0,7,None,0,(.035,.22,.78),2.1)
M["city_warm"]=mat("Tower windows warm",(.8,.43,.18),.25,0,7,None,0,(1.,.36,.09),1.8)
M["city_pink"]=mat("Tower windows pink",(.62,.045,.30),.25,0,7,None,0,(.7,.02,.22),1.9)

def cube(name,loc,scale,material,edge=.02):
    bpy.ops.mesh.primitive_cube_add(size=1,location=loc); o=bpy.context.object; o.name=name
    o.dimensions=scale; bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    if material: o.data.materials.append(material)
    move_to(o); bevel(o,edge,3); return o

def uv_sphere(name,loc,scale,material,segments=24,rings=16):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=segments,ring_count=rings,location=loc)
    o=bpy.context.object; o.name=name; o.scale=scale
    if material: o.data.materials.append(material)
    move_to(o); bpy.ops.object.shade_smooth(); return o

def cylinder(name,loc,radius,depth,material,vertices=32,rotation=None):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices,radius=radius,depth=depth,location=loc)
    o=bpy.context.object; o.name=name
    if rotation: o.rotation_euler=rotation
    if material: o.data.materials.append(material)
    move_to(o); bevel(o,.012,3); return o

def curve_obj(name,points,radius,material,cyclic=False,collection=SCENE_COL):
    cv=bpy.data.curves.new(name,'CURVE'); cv.dimensions='3D'; cv.resolution_u=12
    cv.bevel_depth=radius; cv.bevel_resolution=3
    sp=cv.splines.new('POLY'); sp.points.add(len(points)-1)
    for p,co in zip(sp.points,points): p.co=(co[0],co[1],co[2],1)
    sp.use_cyclic_u=cyclic
    o=bpy.data.objects.new(name,cv); collection.objects.link(o)
    if material: o.data.materials.append(material)
    return o

def area_light(name,loc,target,color,power,size):
    data=bpy.data.lights.new(name,'AREA'); data.energy=power; data.color=color; data.shape='DISK'; data.size=size
    o=bpy.data.objects.new(name,data); LIGHT_COL.objects.link(o); o.location=loc
    o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler(); return o

def point_light(name,loc,color,power,radius=.35):
    data=bpy.data.lights.new(name,'POINT'); data.energy=power; data.color=color; data.shadow_soft_size=radius
    o=bpy.data.objects.new(name,data); LIGHT_COL.objects.link(o); o.location=loc; return o

def add_leaf(name,base,tip,width,material,bend=.08):
    a=Vector(base); b=Vector(tip); axis=b-a; side=axis.cross(Vector((0,0,1)))
    if side.length<.001: side=axis.cross(Vector((0,1,0)))
    side.normalize(); mid=(a+b)*.5+Vector((0,0,bend))
    verts=[a,mid+side*width*.5,b,mid-side*width*.5,mid+Vector((0,0,width*.13))]
    me=bpy.data.meshes.new(name); me.from_pydata(verts,[],[(0,1,4),(1,2,4),(2,3,4),(3,0,4)]); me.materials.append(material)
    o=bpy.data.objects.new(name,me); SCENE_COL.objects.link(o)
    sol=o.modifiers.new("Leaf thickness",'SOLIDIFY'); sol.thickness=.003
    sub=o.modifiers.new("Soft leaf edge",'SUBSURF'); sub.levels=1; sub.render_levels=1
    return o

def plant(name,x,y,z,height=1.25,radius=.35):
    cylinder(name+" | planter",(x,y,z+.19),radius*.62,.38,M["ceramic"])
    cylinder(name+" | soil",(x,y,z+.39),radius*.54,.025,M["wood_dark"])
    for i in range(12):
        a=2*pi*i/12+random.uniform(-.15,.15); h=height*random.uniform(.55,1.)
        r=radius*random.uniform(.5,1.25); base=(x,y,z+.40); tip=(x+cos(a)*r,y+sin(a)*r,z+.40+h)
        curve_obj(name+" | stem",[base,(x+cos(a)*r*.45,y+sin(a)*r*.45,z+.40+h*.52),tip],.010,M["plant"])
        for k in range(4):
            t=.32+k*.14; c=Vector(base).lerp(Vector(tip),t)
            for side in (-1,1):
                aa=a+side*.7; leaf_tip=(c.x+cos(aa)*r*.42,c.y+sin(aa)*r*.42,c.z+.08)
                add_leaf(name+" | leaf",c,leaf_tip,r*.52,M["plant2"] if (i+k)%3==0 else M["plant"],.015)

# --- Room shell and architecture ---
cube("Continuous polished obsidian floor",(0,0,-.095),(W+.12,D+.12,.19),M["stone"],.035)
for ix in range(4):
    for iy in range(5):
        cube(f"Stone floor tile {ix}-{iy}",(-3.15+ix*2.10,-4.25+iy*2.12,.006),(2.07,2.09,.018),M["stone_light"] if (ix+iy)%5==0 else M["stone"],.012)
cube("Left graphite plaster wall",(-4.17,0,2.2),(.18,10.8,4.4),M["wall"],.025)
cube("Right graphite plaster wall",(4.17,0,2.2),(.18,10.8,4.4),M["wall"],.025)
cube("Rear wall above glazing",(0,-5.35,4.12),(8.25,.20,.56),M["wall"],.02)
cube("Rear left window pier",(-4.04,-5.35,2.15),(.28,.24,3.86),M["wall"],.02)
cube("Rear right window pier",(4.04,-5.35,2.15),(.28,.24,3.86),M["wall"],.02)
cube("Rear glazing sill",(0,-5.32,.70),(7.75,.28,.16),M["metal"],.018)
cube("Rear glazing head",(0,-5.32,3.92),(7.75,.26,.16),M["metal"],.018)
for i in range(5):
    cube(f"Panoramic glass panel {i}",(-3.12+i*1.56,-5.43,2.30),(1.52,.018,3.04),M["glass"],.005)
for i,x in enumerate([-3.88,-2.34,-.78,.78,2.34,3.88]):
    cube(f"Window mullion {i}",(x,-5.47,2.30),(.042,.075,3.13),M["metal"],.008)
for z in (.78,3.84): cube(f"Glazing reveal {z}",(0,-5.49,z),(7.72,.05,.028),M["chrome"],.009)
for i in range(28):
    y=-4.55+i*.31
    cube(f"Right wall walnut slat {i}",(4.04,y,2.22),(.085,.105,3.50),M["wood_dark"],.018)
    if i%4==0: cube(f"Slat accent insert {i}",(3.985,y,2.25),(.018,.012,2.90),M["violet"],.004)
cube("Floating ceiling plane",(0,0,4.38),(8.3,10.8,.16),M["wall"],.02)
cube("Recessed ceiling tray dark inset",(0,0,4.285),(6.95,9.05,.045),M["wood_dark"],.012)
for side in (-1,1):
    x=side*3.72; curve_obj("Blue ceiling perimeter LED",[(x,-5.,4.22),(x,5.,4.22)],.018,M["blue"])
    curve_obj("Violet ceiling perimeter LED",[(side*2.95,-4.95,4.22),(side*2.95,4.95,4.22)],.014,M["violet"])
for y in (-4.85,4.85): curve_obj("Ceiling cove cross run",[(-3.72,y,4.22),(3.72,y,4.22)],.016,M["violet"] if y<0 else M["blue"])
for x in (-2.75,-.95,.95,2.75):
    for y in (-3.2,0,3.2):
        cylinder("Downlight machined bezel",(x,y,4.285),.105,.025,M["metal"])
        cylinder("Downlight opal diffuser",(x,y,4.268),.072,.016,M["warm"])
        area_light("Warm ceiling downlight",(x,y,4.1),(x,y,.9),(1.,.74,.52),42,.45)
for side in (-1,1):
    cube("Floating black skirting",(side*4.045,0,.12),(.08,10.7,.18),M["wood_dark"],.015)
    cube("Skirting reveal line",(side*3.995,0,.205),(.018,10.5,.018),M["blue"] if side<0 else M["violet"],.004)

# --- Exterior night skyline with lit windows and harbour reflections ---
cube("Distant harbour water",(0,-12.8,-.14),(64,30,.12),M["city"],0)
for b in range(34):
    bx=-23.+b*1.38+random.uniform(-.38,.38); by=-9.-random.uniform(0,8.5)
    bw=random.uniform(.62,1.28); bh=random.uniform(5.,16.); bd=random.uniform(.6,1.4); top=bh+random.uniform(0,1.5)
    cube(f"Harbour skyline tower {b}",(bx,by,top*.5),(bw,bd,bh),M["city"],.018)
    if b%7==0: curve_obj("Antenna mast",[(bx,by,top),(bx,by,top+1.1)],.018,M["city_cyan"])
    rows=max(4,int(bh/.52)); cols=max(2,int(bw/.22))
    for rr in range(rows):
        for cc in range(cols):
            if random.random()<.58: continue
            wx=bx-bw*.5+.12+cc*(bw-.24)/max(1,cols-1); wz=.42+rr*.43
            wm=random.choice([M["city_cyan"],M["city_warm"],M["city_pink"]])
            cube("Distant lit office window",(wx,by+bd*.5+.012,wz),(.065,.014,.105),wm,.003)
for name,x,y,h,w,d in [("Central spire",-1.1,-9.2,12.5,1.,1.),("Harbour needle",7.3,-10.4,15.8,.85,.9),("Glass slab",-8.,-11.6,10.8,1.7,.8)]:
    cube(name,(x,y,h*.5),(w,d,h),M["city"],.02)
    for row in range(int(h/.48)):
        if row%4==0: continue
        for col in range(3):
            cube(name+" | lit glazing",(x+(col-1)*w*.25,y+d*.5+.015,.55+row*.46),(.075,.012,.13),M["city_warm"] if (row+col)%3 else M["city_cyan"],.002)
for i in range(170):
    x=random.uniform(-26,26); y=random.uniform(-18,-8); length=random.uniform(.18,1.8)
    curve_obj("Harbour reflection",[(x,y,.005),(x+random.uniform(-.12,.12),y+length,.005)],random.uniform(.004,.014),random.choice([M["city_cyan"],M["city_pink"],M["city_warm"]]))

# --- Curtains: pleated mesh rather than flat cards ---
def curtain_panel(name,x0,x1,y,z0,z1,phase):
    verts=[]; faces=[]; nx,nz=44,18
    for iz in range(nz+1):
        v=iz/nz; z=z0+(z1-z0)*v
        for ix in range(nx+1):
            u=ix/nx; x=x0+(x1-x0)*u
            fold=.052*cos(u*2*pi*15+phase)+.012*sin(u*2*pi*31+phase)
            verts.append((x,y+fold,z-.028*sin(u*pi)*v))
    for iz in range(nz):
        for ix in range(nx):
            a=iz*(nx+1)+ix; faces.append((a,a+1,a+nx+2,a+nx+1))
    me=bpy.data.meshes.new(name); me.from_pydata(verts,[],faces); me.materials.append(M["felt"])
    ob=bpy.data.objects.new(name,me); SCENE_COL.objects.link(ob)
    sol=ob.modifiers.new("Fabric thickness",'SOLIDIFY'); sol.thickness=.012
    sub=ob.modifiers.new("Fabric soft folds",'SUBSURF'); sub.levels=1; sub.render_levels=2
curtain_panel("Left tailored velvet curtain",-3.92,-3.20,-5.15,.82,3.82,.2)
curtain_panel("Right tailored velvet curtain",3.20,3.92,-5.15,.82,3.82,1.1)
curve_obj("Curtain track",[(-4.02,-5.10,3.91),(4.02,-5.10,3.91)],.025,M["metal"])

# --- Bedroom: a real upholstered bed, channel headboard, layered soft bedding ---
bx,by=-.92,-3.15
cube("Floating bed plinth",(bx,by,.23),(2.50,3.12,.36),M["wood_dark"],.10)
cube("Bed reveal shadow line",(bx,by,.405),(2.36,2.96,.025),M["violet"],.01)
cube("Pocket sprung mattress",(bx,by,.57),(2.24,2.90,.36),M["fabric_light"],.14)
cube("Mattress piping edge",(bx,by,.735),(2.18,2.82,.028),M["white"],.012)
def duvet_mesh():
    verts=[]; faces=[]; nx,ny=52,64; x0,x1=bx-1.12,bx+1.12; y0,y1=by-1.42,by+1.30
    for j in range(ny+1):
        v=j/ny; y=y0+(y1-y0)*v
        for i in range(nx+1):
            u=i/nx; x=x0+(x1-x0)*u; edge=min(u,1-u,v,1-v)
            z=.795+.018*sin(u*8*pi+v*3*pi)+.012*sin(v*13*pi+u*2*pi)
            z+=.025*sin(u*pi)*sin(v*pi); z-=.10*(1.-min(1.,edge*14.))
            z+=.008*sin((u*17+v*8)*pi)*sin(v*pi); verts.append((x,y,z))
    for j in range(ny):
        for i in range(nx):
            a=j*(nx+1)+i; faces.append((a,a+1,a+nx+2,a+nx+1))
    me=bpy.data.meshes.new("Duvet drape mesh"); me.from_pydata(verts,[],faces); me.materials.append(M["duvet"])
    ob=bpy.data.objects.new("Soft sculpted charcoal duvet",me); SCENE_COL.objects.link(ob)
    sol=ob.modifiers.new("Duvet thickness",'SOLIDIFY'); sol.thickness=.045
    sub=ob.modifiers.new("Fabric smoothing",'SUBSURF'); sub.levels=1; sub.render_levels=2
duvet_mesh()
for yy in (by-.94,by-.56,by-.18,by+.20,by+.58):
    pts=[]
    for i in range(32):
        x=bx-1.+i*2/31; pts.append((x,yy+.012*sin(i*.26),.824+.012*sin(i*.34+yy*2)))
    curve_obj("Duvet stitch line",pts,.0035,M["fabric_light"])
for i in range(3):
    x=bx-.69+i*.69; p=uv_sphere("Large down pillow",(x,by-1.02,.91),(.39,.28,.16),M["fabric_light"],32,20)
    p.rotation_euler[2]=random.uniform(-.10,.10)
    curve_obj("Pillow welt",[(x-.31,by-1.02,.93),(x,by-1.285,.93),(x+.31,by-1.02,.93)],.006,M["duvet"])
for i in range(2):
    x=bx-.40+i*.80; p=uv_sphere("Accent pillow",(x,by-.75,.95),(.32,.20,.14),M["fabric"],32,20)
    p.rotation_euler[2]=(-1 if i==0 else 1)*.08
cube("Bed headboard core",(bx,by-1.57,1.36),(2.52,.20,1.36),M["leather"],.09)
for i in range(9):
    x=bx-1.12+i*.28
    cube(f"Headboard upholstered vertical channel {i}",(x,by-1.687,1.36),(.235,.042,1.17),M["fabric"],.07)
    curve_obj("Headboard channel seam",[(x+.125,by-1.716,.84),(x+.125,by-1.716,1.86)],.003,M["fabric_light"])
curve_obj("Headboard violet backlight",[(bx-1.25,by-1.70,2.07),(bx+1.25,by-1.70,2.07)],.014,M["violet"])
for side in (-1,1):
    nx=bx+side*1.55; ny=by-.83
    cube("Floating walnut nightstand",(nx,ny,.72),(.62,.70,.62),M["wood"],.055)
    cube("Nightstand stone top",(nx,ny,1.045),(.66,.74,.045),M["stone_light"],.025)
    for z in (.58,.80):
        cube("Nightstand drawer front",(nx,ny+.356,z),(.54,.025,.18),M["wood_dark"],.018)
        cube("Brass drawer pull",(nx,ny+.378,z),(.16,.018,.018),M["brass"],.008)
    cylinder("Lamp metal base",(nx,ny,1.10),.15,.045,M["brass"])
    cylinder("Lamp stem",(nx,ny,1.30),.018,.36,M["brass"])
    uv_sphere("Warm opal globe",(nx,ny,1.50),(.14,.14,.17),M["warm"],28,20)
    point_light("Bedside warm practical",(nx,ny,1.52),(1.,.46,.22),28,.28)
    cube("Nightstand book",(nx+side*.12,ny+.05,1.085),(.22,.32,.035),M["fabric"],.008)

# --- Gaming setup on left: ultrawide, real keyboard, glass PC, chair and studio gear ---
dx,dy=-3.12,.42
cube("Gaming desk walnut slab",(-3.02,dy,.79),(1.72,2.48,.10),M["wood"],.045)
cube("Desk edge metal inlay",(-2.99,dy,.755),(1.68,2.40,.025),M["metal"],.012)
curve_obj("Desk front cyan underglow",[(-2.13,dy-1.14,.71),(-2.13,dy+1.14,.71)],.012,M["blue"])
for y in (dy-.82,dy+.82):
    cube("Desk drawer pedestal",(-3.62,y,.39),(.48,.57,.70),M["wood_dark"],.04)
    for z in (.23,.43,.63):
        cube("Desk drawer front",(-3.365,y,z),(.022,.49,.15),M["wood"],.016)
        cube("Drawer finger pull",(-3.345,y,z),(.012,.12,.012),M["brass"],.005)
for y in (dy-1.04,dy+1.04):
    cube("Desk trestle upright",(-2.28,y,.40),(.065,.065,.70),M["metal"],.018)
    cube("Desk foot glide",(-2.28,y,.055),(.55,.11,.045),M["metal"],.015)
cube("Ultrawide monitor aluminium shell",(-3.955,dy,1.73),(.13,2.12,1.13),M["metal"],.05)
cube("Ultrawide black bezel",(-3.878,dy,1.73),(.035,2.02,1.03),M["screen"],.025)
cube("Ultrawide OLED display",(-3.854,dy,1.73),(.012,1.94,.95),M["screen_glow"],.008)
cube("Monitor lower chin",(-3.84,dy,1.18),(.045,2.04,.065),M["wood_dark"],.012)
cube("Monitor logo",(-3.811,dy,1.18),(.012,.11,.018),M["chrome"],.004)
curve_obj("Monitor VESA arm",[(-3.90,dy,1.48),(-3.69,dy,1.42),(-3.69,dy,1.15)],.035,M["metal"])
curve_obj("Monitor braided cable",[(-3.68,dy+.62,1.16),(-3.53,dy+.62,.94),(-3.52,dy+.62,.83)],.009,M["wood_dark"])
cube("Large desk mat",(-2.63,dy+.20,.858),(.78,1.18,.018),M["leather"],.05)
cube("Mechanical keyboard body",(-2.58,dy+.18,.89),(.31,.91,.055),M["wood_dark"],.025)
for row in range(6):
    for col in range(17):
        y=dy-.24+col*.052; x=-2.72+row*.052
        cube("PBT keyboard key",(x,y,.925),(.041,.043,.026),M["fabric_light"] if (row+col)%7 else M["blue"],.008)
cube("Keyboard spacebar",(-2.57,dy+.18,.925),(.035,.27,.024),M["fabric"],.008)
uv_sphere("Ergonomic gaming mouse",(-2.49,dy-.60,.94),(.075,.13,.052),M["leather"])
curve_obj("Mouse RGB seam",[(-2.49,dy-.70,.96),(-2.49,dy-.60,.99),(-2.49,dy-.50,.96)],.006,M["violet"])
cube("PC tower chassis",(-2.72,dy+1.18,1.17),(.62,.56,.66),M["metal"],.045)
cube("PC tempered glass side",(-2.38,dy+1.18,1.17),(.018,.48,.57),M["glass"],.009)
for k in range(3):
    z=.99+k*.18
    cylinder("ARGB cooling fan ring",(-2.365,dy+1.18,z),.075,.014,M["violet"] if k==1 else M["blue"],48,(0,pi/2,0))
    cylinder("ARGB fan hub",(-2.350,dy+1.18,z),.020,.018,M["chrome"],24,(0,pi/2,0))
    for blade in range(5):
        a=2*pi*blade/5
        uv_sphere("PC fan blade",(-2.35,dy+1.18+cos(a)*.039,z+sin(a)*.039),(.012,.014,.034),M["screen_glow"],12,8)
cube("GPU backplate",(-2.35,dy+1.0,1.10),(.025,.35,.07),M["wood_dark"],.01)
for i in range(3): cube("RGB memory module",(-2.35,dy+1.30+i*.045,1.32),(.02,.022,.18),M["blue"],.006)
for y in (dy-.98,dy+.96):
    cube("Nearfield speaker cabinet",(-2.68,y,1.04),(.27,.22,.34),M["wood_dark"],.028)
    uv_sphere("Speaker woofer",(-2.52,y,1.01),(.018,.073,.073),M["leather"])
    uv_sphere("Speaker tweeter",(-2.52,y,1.14),(.018,.035,.035),M["chrome"])
curve_obj("Microphone boom arm",[(-2.8,dy-.98,1.),(-2.47,dy-.95,1.42),(-2.2,dy-.62,1.45)],.018,M["metal"])
cylinder("Studio microphone",(-2.2,dy-.62,1.39),.045,.20,M["leather"],24,(pi/2,0,0))
chair_x,chair_y=-1.68,dy-.10
cylinder("Chair gas lift",(chair_x,chair_y,.42),.055,.54,M["chrome"])
uv_sphere("Chair five star hub",(chair_x,chair_y,.18),(.16,.16,.10),M["metal"])
for i in range(5):
    a=2*pi*i/5; px=chair_x+cos(a)*.46; py=chair_y+sin(a)*.46
    curve_obj("Chair aluminium spoke",[(chair_x,chair_y,.18),(px,py,.12)],.035,M["metal"])
    cylinder("Chair caster wheel",(px,py,.095),.075,.09,M["leather"],24,(pi/2,0,0))
uv_sphere("Gaming chair seat cushion",(chair_x,chair_y,.68),(.43,.48,.15),M["leather"],32,20)
uv_sphere("Gaming chair seat insert",(chair_x+.01,chair_y,.77),(.32,.36,.075),M["fabric"],32,20)
back=uv_sphere("Gaming chair tall backrest",(chair_x+.30,chair_y-.03,1.28),(.17,.48,.70),M["leather"],32,24); back.rotation_euler[1]=-.18
inset=uv_sphere("Backrest centre mesh",(chair_x+.18,chair_y-.03,1.28),(.065,.36,.54),M["fabric"],32,24); inset.rotation_euler[1]=-.18
for side in (-1,1):
    uv_sphere("Chair shoulder bolster",(chair_x+.30,chair_y+side*.38,1.43),(.17,.12,.39),M["leather"],24,16)
    curve_obj("Chair arm support",[(chair_x,chair_y+side*.39,.73),(chair_x+.22,chair_y+side*.39,.90)],.035,M["metal"])
    uv_sphere("Chair arm pad",(chair_x+.20,chair_y+side*.39,.93),(.26,.09,.065),M["leather"])
uv_sphere("Chair lumbar pillow",(chair_x+.15,chair_y-.02,1.06),(.085,.24,.13),M["fabric_light"])

# --- Lounge sectional, rug, glass coffee table and styling ---
sx,sy=1.55,2.20
cube("Sectional upholstered base",(sx,sy,.42),(1.45,2.75,.42),M["leather"],.16)
cube("Sectional chaise return",(sx-.18,sy-1.07,.42),(1.80,.70,.42),M["leather"],.14)
for i in range(3):
    y=sy-.91+i*.91
    uv_sphere("Sectional deep seat cushion",(sx+.20,y,.69),(.52,.43,.19),M["fabric"],32,24)
    curve_obj("Seat cushion welt",[(sx-.17,y-.39,.73),(sx+.20,y-.42,.73),(sx+.58,y-.39,.73)],.006,M["fabric_light"])
for i in range(3):
    y=sy-.90+i*.91; p=uv_sphere("Sectional back cushion",(sx-.42,y,1.03),(.24,.43,.43),M["fabric_light"],32,24); p.rotation_euler[1]=-.12
    curve_obj("Back cushion seam",[(sx-.61,y-.33,1.02),(sx-.62,y,1.34),(sx-.61,y+.33,1.02)],.006,M["fabric"])
uv_sphere("Draped lounge throw",(sx+.18,sy+1.10,.89),(.42,.38,.10),M["duvet"],32,16)
cube("Large woven area rug",(1.05,1.25,.035),(3.25,3.40,.055),M["rug"],.05)
for i in range(6):
    yy=-.30+i*.62; curve_obj("Rug woven border line",[(-.50,yy,.067),(2.65,yy,.067)],.003,M["fabric_light"])
cube("Floating coffee table glass top",(1.05,1.50,.47),(1.12,1.65,.075),M["stone_light"],.07)
cube("Coffee table lower shelf",(1.05,1.50,.24),(.92,1.43,.055),M["wood_dark"],.025)
for x in (.62,1.48):
    for y in (.88,2.12): cylinder("Coffee table brushed leg",(x,y,.32),.027,.28,M["brass"])
for j in range(3): cube("Coffee table design book",(.80+j*.035,1.10+j*.02,.535+j*.035),(.42,.28,.035),M["fabric_light"] if j%2 else M["wood_dark"],.008)
cube("Black stone catchall tray",(1.35,1.67,.53),(.34,.48,.035),M["ceramic"],.03)
cylinder("Scented candle glass",(1.36,1.72,.62),.07,.15,M["glass"])
cylinder("Candle wax",(1.36,1.72,.70),.055,.012,M["white"])
uv_sphere("Candle flame",(1.36,1.72,.73),(.014,.014,.035),M["warm"],16,12)
point_light("Candle glow",(1.36,1.72,.76),(1.,.35,.10),4,.12)
plant("Coffee table succulent",.70,1.95,.54,.26,.13)

# --- Right-wall OLED/media console ---
tvx,tvy=3.88,-.12
cube("85 inch OLED TV chassis",(tvx,tvy,2.08),(.12,2.42,1.46),M["metal"],.045)
cube("TV black glass bezel",(tvx-.071,tvy,2.08),(.018,2.34,1.38),M["screen"],.025)
cube("OLED cinematic display",(tvx-.083,tvy,2.08),(.008,2.25,1.29),M["screen_glow"],.012)
cube("TV bottom status bar",(tvx-.09,tvy,1.36),(.009,.58,.018),M["blue"],.004)
curve_obj("TV bias lighting",[(3.79,tvy-1.13,1.28),(3.79,tvy+1.13,1.28),(3.79,tvy+1.13,2.88),(3.79,tvy-1.13,2.88)],.014,M["blue"],True)
cube("Floating media console cabinet",(3.72,tvy,.62),(.54,3.05,.52),M["wood_dark"],.055)
cube("Media console walnut top",(3.68,tvy,.90),(.66,3.12,.07),M["wood"],.025)
for i in range(4):
    y=tvy-1.13+i*.75
    cube("Media console drawer",(3.433,y,.60),(.025,.69,.36),M["wood"],.025)
    cube("Media drawer brass pull",(3.414,y,.60),(.012,.12,.018),M["brass"],.005)
cube("Atmos soundbar",(3.77,tvy,1.06),(.20,1.62,.09),M["metal"],.025)
for i in range(18): cylinder("Soundbar perforation",(3.66,tvy-.75+i*.088,1.06),.009,.014,M["chrome"],12,(0,pi/2,0))
cube("Console streaming box",(3.55,tvy-.95,1.02),(.30,.34,.08),M["wood_dark"],.018)
uv_sphere("Wireless controller",(3.45,tvy+.96,1.04),(.16,.25,.055),M["leather"])
for side in (-1,1): cylinder("Controller thumbstick",(3.39,tvy+.96+side*.07,1.095),.025,.018,M["chrome"])

# --- Decor, shelves, abstract artwork and indoor greenery ---
cube("Framed abstract artwork body",(-3.63,-4.98,2.34),(.06,.70,1.15),M["metal"],.025)
cube("Abstract artwork dark ground",(-3.592,-4.94,2.34),(.016,.62,1.07),M["screen"],.008)
for i in range(7):
    z=1.93+i*.12
    curve_obj("Artwork light trace",[(-3.58,-5.18,z),(-3.58,-4.95,z+.04*sin(i)),(-3.58,-4.72,z-.05)],.007,M["blue"] if i%2 else M["violet"])
for z,y in ((2.55,-.68),(3.12,-.50)):
    cube("Floating black display shelf",(-3.92,y,z),(.30,2.0,.065),M["wood_dark"],.025)
    curve_obj("Shelf underglow",[(-3.78,y-.94,z-.05),(-3.78,y+.94,z-.05)],.010,M["violet"])
    for k in range(4):
        yy=y-.72+k*.46
        cube("Shelf collectible plinth",(-3.76,yy,z+.10),(.18,.22,.08),M["metal"],.015)
        uv_sphere("Shelf collectible sculpture",(-3.72,yy,z+.24),(.10,.09,.12),M["chrome"] if k%2 else M["ceramic"])
plant("Tall window-side ficus",-3.55,-3.82,.10,1.65,.38)
plant("TV-side indoor palm",3.26,-3.12,.10,1.72,.42)
plant("Lounge floor plant",2.95,3.10,.10,1.30,.32)
for y in (-4.2,-2.3,-.4,1.5,3.4):
    cylinder("Slat wall uplight base",(3.89,y,.12),.09,.05,M["metal"])
    point_light("Slat wall grazing light",(3.78,y,.23),(.25,.10,1.),13,.24)
for x,material in ((-3.84,M["blue"]),(3.84,M["violet"])):
    curve_obj("Window vertical accent light",[(x,-5.45,.80),(x,-5.45,3.83)],.012,material)
cube("Entry floating shelf",(2.9,4.50,1.25),(.75,.20,.09),M["wood"],.025)
cylinder("Entry sculptural vase",(2.9,4.50,1.45),.10,.32,M["ceramic"])

# --- Room lighting: soft key + controlled colored bounce ---
area_light("Large soft ceiling key",(0,1.,4.15),(0,-1.,.6),(.52,.62,1.),370,5.)
area_light("Blue desk-side wash",(-3.65,.45,3.40),(-1.5,.7,1.),(.12,.30,1.),220,2.2)
area_light("Violet lounge-side wash",(3.60,1.5,3.45),(1.4,1.4,.9),(.46,.12,1.),200,2.3)
area_light("Warm bedroom bounce",(-.9,-4.3,3.5),(-.9,-2.8,.9),(1.,.56,.31),95,1.8)
area_light("Cool window fill",(0,-5.1,3.55),(0,0,1.25),(.20,.34,1.),240,4.5)
area_light("Soft camera-side fill",(0,4.5,3.),(0,0,1.3),(.38,.44,.72),120,4.)

# --- Three camera views ---
def look_at(obj,point): obj.rotation_euler=(Vector(point)-obj.location).to_track_quat('-Z','Y').to_euler()
def camera(name,loc,target,lens=27):
    data=bpy.data.cameras.new(name); data.lens=lens; data.sensor_width=36
    o=bpy.data.objects.new(name,data); CAM_COL.objects.link(o); o.location=loc; look_at(o,target); return o
cam_wide=camera("01 | Full penthouse hero",(.20,4.95,2.15),(0.,-1.35,1.95),25)
cam_desk=camera("02 | Gaming studio detail",(-.75,2.95,1.75),(-3.12,.35,1.55),34)
cam_lounge=camera("03 | Lounge and bedroom",(2.55,4.65,1.75),(1.65,-1.05,1.75),30)

scene=bpy.context.scene
scene.render.engine='CYCLES'; scene.cycles.samples=32; scene.cycles.use_denoising=True; scene.cycles.use_adaptive_sampling=True; scene.cycles.adaptive_threshold=0.02; scene.cycles.max_bounces=6; scene.cycles.preview_samples=16
scene.render.resolution_x=1600; scene.render.resolution_y=1000; scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG'; scene.render.image_settings.color_mode='RGBA'; scene.render.image_settings.color_depth='8'
scene.view_settings.view_transform='AgX'; scene.view_settings.look='AgX - Medium High Contrast'
scene.view_settings.exposure=.35; scene.view_settings.gamma=1.
scene.world.use_nodes=True
scene.world.node_tree.nodes['Background'].inputs['Color'].default_value=(.008,.014,.035,1)
scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value=.24
scene.render.filepath=os.path.join(RENDER_DIR,"01_penthouse_wide.png"); scene.camera=cam_wide
bpy.ops.render.render(write_still=True)
for cam,filename in ((cam_desk,"02_gaming_desk_detail.png"),(cam_lounge,"03_lounge_bedroom_detail.png")):
    scene.camera=cam; scene.render.filepath=os.path.join(RENDER_DIR,filename); bpy.ops.render.render(write_still=True)
scene.camera=cam_wide; scene.render.filepath=os.path.join(RENDER_DIR,"01_penthouse_wide.png")
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT,"skyline_penthouse.blend"))

# GLB interchange export. The Blender project itself remains the editable source of truth.
bpy.ops.object.select_all(action='DESELECT')
for ob in SCENE_COL.objects: ob.select_set(True)
bpy.context.view_layer.objects.active=next((o for o in SCENE_COL.objects if o.type=='MESH'),None)
bpy.ops.export_scene.gltf(filepath=os.path.join(OUT,"skyline_penthouse.glb"),export_format='GLB',
                          use_selection=True,export_apply=True,export_yup=True,
                          export_materials='EXPORT',export_cameras=False,export_lights=False)

# Runtime QBX mesh: grouped triangles with base tint and existing Quest PBR material profile ID.
# Header <8sIII>, batch <i4fI>, vertex <5f> (Quest XYZ + UV). Blender (X,Z,Y) remap flips winding.
mat_batches={}; depsgraph=bpy.context.evaluated_depsgraph_get()
for ob in list(SCENE_COL.objects):
    if ob.type not in {'MESH','CURVE','FONT','SURFACE'}: continue
    ev=ob.evaluated_get(depsgraph)
    try: mesh=ev.to_mesh()
    except Exception: continue
    if not mesh: continue
    mesh.calc_loop_triangles()
    uv_layer=mesh.uv_layers.active.data if mesh.uv_layers.active else None
    for tri in mesh.loop_triangles:
        mi=tri.material_index; material=mesh.materials[mi] if mi<len(mesh.materials) else None
        if not material: continue
        sid=int(material.get('qbx_shader_id',2))
        bs=material.node_tree.nodes.get('Principled BSDF') if material.use_nodes else None
        col=tuple(bs.inputs['Base Color'].default_value) if bs and 'Base Color' in bs.inputs else tuple(material.diffuse_color)
        key=(material.name,sid,tuple(round(float(c),5) for c in col[:4]))
        batch=mat_batches.setdefault(key,[])
        for li in (tri.loops[0],tri.loops[2],tri.loops[1]):
            loop=mesh.loops[li]; v=ev.matrix_world @ mesh.vertices[loop.vertex_index].co
            uv=uv_layer[li].uv if uv_layer else (0.,0.)
            # Blender UV values may be Vector-like or plain tuples (notably the no-UV fallback).
            # Indexing supports both representations and avoids tuple.x/tuple.y failures.
            batch.extend((float(v.x),float(v.z),float(v.y),float(uv[0]),float(uv[1])))
    ev.to_mesh_clear()

batches=[]; total=0
for (name,sid,col),verts in mat_batches.items():
    if not verts: continue
    count=len(verts)//5
    if count<3: continue
    batches.append((sid,col,count,verts)); total+=count
qbx_path=os.path.join(OUT,"skyline_penthouse.qbxmesh")
with open(qbx_path,'wb') as f:
    f.write(struct.pack('<8sIII',b'QBXMSH1\0',1,len(batches),total))
    for sid,col,count,verts in batches:
        f.write(struct.pack('<i4fI',sid,float(col[0]),float(col[1]),float(col[2]),float(col[3]),count))
        f.write(struct.pack('<'+'f'*len(verts),*verts))
print(f"QBX mesh: {len(batches)} material batches, {total} triangle vertices, {os.path.getsize(qbx_path):,} bytes")
print("OUTPUT_DIR="+OUT)
