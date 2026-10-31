#!/usr/bin/env python3
"""Render a complete QuestBridge XR bedroom tour from the actual current GLB assets."""
import argparse, json, math, sys
from pathlib import Path
import bpy
from mathutils import Vector

PI=math.pi
HW,HD,RH=4.2,5.4,4.4
BED_X,BED_Z=HW-2.0,-HD+2.10
DESK_X,DESK_Z=-HW+1.32,-HD+.43
DESK_Y,MON_Z=.75,(-HD+.43)-.31
TV_Z,WIN_Z=BED_Z+3.18,-HD+2.10
LED_MATS,LED_LIGHTS,MONITOR_RIGS={},{},[]

THEMES=[
("01_cyber_blue_violet","CYBER BLUE + VIOLET",[
((.03,.35,1),(.52,.08,1)),((.37,.08,1),(.04,.50,1)),((.02,.20,1),(.03,.58,1)),
((.42,.06,1),(.10,.26,1)),((.02,.70,.9),(.22,.12,1)),((.04,.28,1),(.02,.68,1)),((.55,.08,1),(.10,.22,1))]),
("02_pink_cyan","PINK + CYAN",[
((1,.045,.34),(.02,.88,1)),((1,.06,.48),(.03,.78,1)),((.98,.04,.54),(.03,.78,1)),
((.04,.80,1),(.92,.04,.58)),((.90,.06,.65),(.02,.85,1)),((1,.08,.38),(.10,.76,1)),((.05,.88,1),(.94,.08,.58))]),
("03_emerald_gold","EMERALD + GOLD",[
((.03,.88,.34),(1,.48,.055)),((.02,.58,.26),(.96,.56,.08)),((.02,.78,.4),(.4,.86,.12)),
((1,.46,.06),(.9,.78,.1)),((.04,.72,.5),(1,.42,.08)),((.04,.76,.32),(.7,.94,.1)),((1,.5,.08),(.88,.78,.18))]),
("04_arctic_ice","ARCTIC ICE",[
((.65,.9,1),(.1,.4,1)),((.82,.95,1),(.22,.56,1)),((.26,.72,1),(.8,.94,1)),
((.48,.52,1),(.88,.94,1)),((.24,.78,1),(.9,.95,1)),((.58,.86,1),(.96,.98,1)),((.32,.58,1),(.86,.94,1))]),
("05_sunset_amber","SUNSET AMBER",[
((1,.25,.045),(1,.05,.34)),((1,.34,.06),(1,.12,.30)),((1,.19,.06),(1,.4,.08)),
((1,.08,.24),(.96,.28,.08)),((1,.28,.06),(1,.07,.4)),((1,.34,.08),(1,.10,.28)),((1,.08,.32),(1,.5,.08))])
]

def cli():
    av=sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else []
    p=argparse.ArgumentParser()
    p.add_argument("--bed",required=True);p.add_argument("--desk",required=True)
    p.add_argument("--monitor",required=True);p.add_argument("--output-dir",required=True)
    return p.parse_args(av)

def reset():
    bpy.ops.object.select_all(action="SELECT");bpy.ops.object.delete(use_global=False)
    for data in (bpy.data.materials,bpy.data.meshes,bpy.data.curves,bpy.data.cameras,bpy.data.lights):
        for d in list(data):
            if d.users==0:data.remove(d)
    LED_MATS.clear();LED_LIGHTS.clear();MONITOR_RIGS.clear()

def material(name,c,r=.45,m=0,t=0):
    a=bpy.data.materials.new(name);a.diffuse_color=(*c,1);a.use_nodes=True
    b=a.node_tree.nodes.get("Principled BSDF")
    b.inputs["Base Color"].default_value=(*c,1);b.inputs["Roughness"].default_value=r;b.inputs["Metallic"].default_value=m
    if "Transmission Weight" in b.inputs:b.inputs["Transmission Weight"].default_value=t
    return a

def emissive(name,c,s=3):
    a=bpy.data.materials.new(name);a.use_nodes=True;a.node_tree.nodes.clear()
    out=a.node_tree.nodes.new("ShaderNodeOutputMaterial");e=a.node_tree.nodes.new("ShaderNodeEmission")
    e.inputs["Color"].default_value=(*c,1);e.inputs["Strength"].default_value=s
    a.node_tree.links.new(e.outputs[0],out.inputs["Surface"]);return a

def set_emission(a,c,s=3):
    e=next((n for n in a.node_tree.nodes if n.type=="EMISSION"),None)
    if e:e.inputs["Color"].default_value=(*c,1);e.inputs["Strength"].default_value=s

def cube(n,loc,dims,ma,bev=.015,seg=3):
    bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bpy.context.object;o.name=n;o.dimensions=dims
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    if ma:o.data.materials.append(ma)
    if bev>0:
        b=o.modifiers.new("Soft machined edges","BEVEL");b.width=min(bev,min(dims)*.44);b.segments=seg
        o.modifiers.new("Weighted normals","WEIGHTED_NORMAL")
    return o

def cylinder(n,loc,dia,depth,ma,verts=48,bev=.006,rot=None):
    bpy.ops.mesh.primitive_cylinder_add(vertices=verts,radius=dia*.5,depth=depth,location=loc)
    o=bpy.context.object;o.name=n
    if ma:o.data.materials.append(ma)
    if rot:o.rotation_euler=rot
    if bev>0:
        b=o.modifiers.new("Soft rim","BEVEL");b.width=min(bev,dia*.12,depth*.22);b.segments=3
        o.modifiers.new("Weighted normals","WEIGHTED_NORMAL")
    return o

def sphere(n,loc,scale,ma):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=28,ring_count=18,radius=1,location=loc)
    o=bpy.context.object;o.name=n;o.scale=scale;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    if ma:o.data.materials.append(ma)
    for p in o.data.polygons:p.use_smooth=True
    return o

def curve(n,pts,r,ma):
    d=bpy.data.curves.new(n,"CURVE");d.dimensions="3D";d.resolution_u=12;d.bevel_depth=r;d.bevel_resolution=3
    s=d.splines.new("POLY");s.points.add(len(pts)-1)
    for p,v in zip(s.points,pts):p.co=(*v,1)
    o=bpy.data.objects.new(n,d);bpy.context.collection.objects.link(o)
    if ma:o.data.materials.append(ma)
    return o

def light(n,loc,target,color,power,size,size_y=None):
    d=bpy.data.lights.new(n,"AREA");d.energy=power;d.shape="RECTANGLE" if size_y else "DISK";d.size=size
    if size_y:d.size_y=size_y
    d.color=color;o=bpy.data.objects.new(n,d);bpy.context.collection.objects.link(o);o.location=loc
    o.rotation_euler=(Vector(target)-o.location).to_track_quat("-Z","Y").to_euler()
    return o

def import_asset(path,loc,name):
    before=set(bpy.data.objects);bpy.ops.import_scene.gltf(filepath=str(path));objs=list(set(bpy.data.objects)-before)
    if not objs:raise RuntimeError("Empty GLB import: "+str(path))
    root=bpy.data.objects.new(name,None);bpy.context.collection.objects.link(root);root.location=loc
    known=set(objs)
    for o in objs:
        if o.parent not in known:
            w=o.matrix_world.copy();o.parent=root;o.matrix_world=w
    bpy.context.view_layer.update()
    return root,objs

def curved_screen(n,cx,y,z,w,h,depth,ma):
    seg=32;v=[];f=[]
    for row in range(2):
        yy=y+(0.5-row)*h
        for i in range(seg+1):
            u=i/seg;v.append((cx+(u-.5)*w,yy,z+depth*(1-(2*u-1)**2)))
    for i in range(seg):f.append((i,i+1,seg+2+i,seg+1+i))
    mesh=bpy.data.meshes.new(n+"_mesh");mesh.from_pydata(v,[],f);mesh.materials.append(ma);mesh.update()
    o=bpy.data.objects.new(n,mesh);bpy.context.collection.objects.link(o)
    s=o.modifiers.new("Glass thickness","SOLIDIFY");s.thickness=.003
    return o

def area_gradient(a,b,t):return tuple(a[i]*(1-t)+b[i]*t for i in range(3))

def add_led_zone(zone,count,fn,strength=4):
    mats=[]
    for i in range(count):
        loc,dims=fn(i,count);m=emissive(f"LED_{zone}_{i:02d}",(.05,.3,1),strength)
        cube(f"LED_Channel_{zone}_{i:02d}",loc,dims,m,.001,2);mats.append(m)
    LED_MATS[zone]=mats

def apply_theme(theme):
    for z,(a,b) in enumerate(theme[2]):
        for i,m in enumerate(LED_MATS.get(z,[])):
            set_emission(m,area_gradient(a,b,(i+.5)/len(LED_MATS[z])),4.5 if z==0 else 3.7)
        for l in LED_LIGHTS.get(z,[]):l.data.color=area_gradient(a,b,.5)
    for side,z in (("L",5),("R",6)):
        m=bpy.data.materials.get(f"Bedside_RGB_{side}")
        if m:set_emission(m,area_gradient(*theme[2][z],.5),2.4)

def add_leds():
    # The headboard emitter runs behind the projecting inner/outer bezel lips.
    def head(i,n):
        q=i/(n/4);k=i%(n//4);t=(k+.5)/(n//4)
        if q<1:return ((BED_X-1.05+2.1*t,.51,BED_Z-1.45),(2.1/(n//4)*1.05,.009,.010))
        if q<2:return ((BED_X-1.05+2.1*t,1.84,BED_Z-1.45),(2.1/(n//4)*1.05,.009,.010))
        if q<3:return ((BED_X-1.065,.51+1.33*t,BED_Z-1.45),(.010,1.33/(n//4)*1.05,.010))
        return ((BED_X+1.065,.51+1.33*t,BED_Z-1.45),(.010,1.33/(n//4)*1.05,.010))
    add_led_zone(0,96,head,4.8)
    add_led_zone(1,48,lambda i,n:((-3.21+6.42*(i+.5)/n,4.10,0),(6.42/n*1.04,.012,.022)))
    for z,x in ((2,-1),(3,1)):
        add_led_zone(z,44,lambda i,n,x=x:((x*(HW-.125),2.3,-HD*.82+(i+.5)*HD*1.64/n),(.010,.020,HD*1.64/n*1.05)))
    add_led_zone(4,40,lambda i,n:((-HW*.8+HW*1.6*(i+.5)/n,4.02,-HD+.14),(HW*1.6/n*1.04,.012,.02)))
    for z,x in ((5,BED_X-1.53),(6,BED_X+1.53)):
        add_led_zone(z,20,lambda i,n,x=x:((x+math.cos(2*PI*(i+.5)/n)*.09,1.045,BED_Z-.94+math.sin(2*PI*(i+.5)/n)*.09),(.02,.012,.02)))
    for z,loc,tgt,power,size in [
       (0,(BED_X,1.45,BED_Z-1.55),(BED_X,1.02,BED_Z-.84),62,1.7),
       (1,(0,4.05,-.2),(0,3.35,-.2),48,3),
       (2,(-4.04,2.3,-.2),(-3.2,2.1,-.2),34,1.2),
       (3,(4.04,2.3,-.2),(3.2,2.1,-.2),34,1.2),
       (4,(0,4,-5.15),(0,3.5,-4.5),36,1.3),
       (5,(BED_X-1.53,1.04,BED_Z-.94),(BED_X-1.53,.72,BED_Z-.94),22,.75),
       (6,(BED_X+1.53,1.04,BED_Z-.94),(BED_X+1.53,.72,BED_Z-.94),22,.75)]:
        l=light(f"LED_BOUNCE_{z}",loc,tgt,area_gradient(*THEMES[0][2][z],.5),power,size,.18)
        LED_LIGHTS.setdefault(z,[]).append(l)

def make_plant():
    x,z=-HW+.62,HD-.44
    pot=material("Planter_Ceramic",(.07,.085,.13),.28,.08);leaf=material("Plant_Deep_Green",(.035,.19,.10),.82)
    cylinder("Planter_Body",(x,.24,z),.38,.48,pot,64,.02)
    cylinder("Planter_Lip",(x,.49,z),.42,.055,material("Planter_Lip_Material",(.45,.38,.28),.55),64,.01)
    cylinder("Planter_Soil",(x,.53,z),.31,.016,material("Planter_Soil_Material",(.08,.055,.035),.95),48,.002)
    for i in range(7):
        a=2*PI*i/7;dx,dz=math.cos(a),math.sin(a);tip=(x+dx*.28,1.35+.12*(i%3),z+dz*.28)
        curve(f"Plant_Stem_{i}",[(x,.54,z),tip],.012,material("Stem_Brown",(.1,.07,.035),.9))
        o=sphere(f"Plant_Leaf_{i}",tip,(.11,.23,.055),leaf);o.rotation_euler[1]=-a
        p=(tip[0]+dx*.15,tip[1]-.15,tip[2]+dz*.15);o=sphere(f"Plant_Leaf_Tip_{i}",p,(.1,.16,.05),leaf);o.rotation_euler[1]=-a

def make_floorlamp():
    x,z=-HW+.30,-1.45
    steel=material("Floor_Lamp_Steel",(.22,.27,.39),.24,.78);wood=material("Floor_Lamp_Wood",(.17,.09,.05),.3)
    cylinder("Floor_Lamp_Base",(x,.055,z),.34,.11,material("Floor_Lamp_Base_Mat",(.015,.02,.035),.28,.3),64,.018)
    cylinder("Floor_Lamp_Stem",(x,.88,z),.038,1.60,steel,32,.003)
    cylinder("Floor_Lamp_Neck",(x,1.70,z),.105,.11,wood,48,.008)
    cylinder("Floor_Lamp_Shade",(x,1.91,z),.43,.42,material("Lamp_Shade_Frosted",(.72,.68,.60),.42),64,.025)
    cylinder("Floor_Lamp_RGB_Diffuser",(x,1.70,z),.255,.035,emissive("Floor_Lamp_RGB",(.15,.45,1),2),64,.004)
    cylinder("Floor_Lamp_Cap",(x,2.115,z),.18,.035,wood,48,.004)
    sphere("Floor_Lamp_Glow",(x,1.84,z),(.12,.09,.12),emissive("Floor_Lamp_Glow_Mat",(.15,.45,1),2))

def make_bedside():
    base=material("Bedside_Charcoal",(.018,.024,.043),.3,.16)
    top=material("Bedside_Satin",(.12,.17,.26),.24,.72)
    detail=material("Bedside_Detail_Metal",(.18,.23,.34),.24,.7)
    for side in (-1,1):
        x,z=BED_X+side*1.53,BED_Z-.94
        cube(f"Bedside_Cabinet_{side}",(x,.405,z),(.52,.60,.56),base,.035)
        cube(f"Bedside_Top_{side}",(x,.68,z),(.56,.055,.60),top,.02)
        for i in range(2):
            y=.34+i*.22
            cube(f"Bedside_Drawer_{side}_{i}",(x,y,z+.289),(.445,.185,.025),material(f"Drawer_{side}_{i}",(.035,.05,.082),.3,.16),.008)
            cube(f"Bedside_Pull_{side}_{i}",(x,y+.035,z+.309),(.10,.012,.012),detail,.004)
        cylinder(f"Bedside_Lamp_Stem_{side}",(x,.79,z),.055,.20,detail,36,.005)
        key="L" if side<0 else "R";lm=emissive(f"Bedside_RGB_{key}",(.12,.42,1),2.4)
        cylinder(f"Bedside_Lamp_Diffuser_{side}",(x,.925,z),.25,.20,lm,48,.008)
        cylinder(f"Bedside_Lamp_Cap_{side}",(x,1.035,z),.17,.025,detail,48,.003)
        sphere(f"Bedside_Lamp_Halo_{side}",(x,.91,z),(.07,.075,.07),lm)

def make_tv():
    x,z=BED_X,TV_Z;w=2.35;h=w*9/16;y=1.95
    frame=material("TV_Black_Frame",(.012,.018,.032),.24,.35)
    screen=emissive("TV_Night_Display",(.015,.035,.10),.65)
    cube("Bedroom_TV_Frame",(x,y,z),(w+.14,h+.14,.085),frame,.025,5)
    cube("Bedroom_TV_Display",(x,y,z-.046),(w,h,.008),screen,.003)
    cab=material("TV_Console_Carcass",(.012,.018,.034),.28,.25);cz=z+.27
    cube("TV_Media_Console",(x,.30,cz),(w*.88,.50,.50),cab,.04)
    cube("TV_Console_Front",(x,.345,cz-.238),(w*.88-.08,.39,.018),material("TV_Console_Satin",(.035,.05,.085),.32,.14),.012)
    for i in range(3):
        px=x+(i-1)*(w*.88)*.29
        cube(f"TV_Console_Bay_{i}",(px,.345,cz-.25),(w*.88*.255,.35,.008),material(f"TV_Bay_{i}",(.045,.06,.11),.37),.009)
    for side in (-1,1):
        sx=x+side*w*.22
        cube(f"TV_Stand_Leg_{side}",(sx,.91,z+.085),(.035,.78,.045),material("TV_Stand_Silver",(.22,.27,.39),.22,.78),.008)
        cube(f"TV_Stand_Foot_{side}",(sx,.55,z+.085),(.14,.035,.34),cab,.008)

def make_door():
    front=HD+.02;wood=material("Door_Walnut",(.16,.09,.05),.28,.08);inset=material("Door_Recess",(.08,.047,.03),.42);brass=material("Door_Brass",(.56,.36,.13),.22,.78)
    w,h,yaw=1.28,2.24,-.70;hx=.66;dx=hx-math.cos(yaw)*w*.5;dz=front-math.sin(-yaw)*w*.5;cz=front+.085
    for side in (-1,1):
        cube(f"Door_Jamb_{side}",(side*.765,1.16,cz),(.115,2.42,.15),wood,.025)
        cube(f"Door_Trim_{side}",(side*.695,1.14,front-.02),(.055,2.30,.11),material("Door_Trim_Inlay",(.12,.14,.2),.35,.3),.015)
    cube("Door_Header",(0,2.365,cz),(1.62,.14,.15),wood,.025)
    cube("Door_Threshold",(0,.035,front-.045),(1.48,.07,.24),material("Door_Threshold_Wood",(.34,.28,.22),.4),.02)
    leaf=cube("Ajar_Door",(dx,h*.5,dz),(w,h,.078),wood,.022);leaf.rotation_euler[1]=yaw
    def fp(lx,ly,lz):return(dx+math.cos(yaw)*lx+math.sin(yaw)*lz,ly,dz-math.sin(yaw)*lx+math.cos(yaw)*lz)
    for i in range(2):
        py=1.68 if i==0 else .61
        for e in (-1,1):
            o=cube(f"Door_Stile_{i}_{e}",fp(e*.455,py,-.066),(.045,.88,.02),wood,.008);o.rotation_euler[1]=yaw
            o=cube(f"Door_Rail_{i}_{e}",fp(0,py+e*.415,-.066),(.88,.045,.02),wood,.008);o.rotation_euler[1]=yaw
        cube(f"Door_Recess_{i}",fp(0,py,-.05),(.80,.76,.018),inset,.025)
    for i in range(3):
        o=cube(f"Door_Hinge_{i}",fp(.595,.36+i*.75,-.006),(.075,.17,.025),brass,.008);o.rotation_euler[1]=yaw
    o=cube("Door_Handle_Plate",fp(-.48,1.12,-.067),(.085,.18,.018),brass,.012);o.rotation_euler[1]=yaw
    sphere("Door_Handle_Knob",fp(-.48,1.12,-.105),(.045,.045,.045),brass)
    x,y=1.12,1.42
    cube("Smart_Switch_Bezel",(x,y,front-.082),(.19,.31,.038),material("Smart_Switch_Satin",(.13,.16,.23),.25,.5),.028)
    cube("Smart_Switch_Glass",(x,y,front-.105),(.158,.276,.012),material("Smart_Switch_Glass",(.025,.034,.055),.16,.35),.018)
    cube("Smart_Switch_Rocker",(x,y+.03,front-.125),(.064,.132,.014),material("Smart_Switch_Rocker",(.29,.34,.43),.22,.3),.012)
    sphere("Smart_Switch_Status",(x+.052,y+.105,front-.133),(.01,.01,.01),emissive("Switch_Amber_Status",(1,.58,.16),2.4))

def make_window():
    x,z=HW-.09,WIN_Z;w,h=min(3.5,HD*.78),2.15;y=2.4
    frame=material("Window_Black_Aluminium",(.025,.035,.055),.22,.72);glass=material("Window_Tinted_Glass",(.018,.06,.12),.1,.34,.82)
    cube("Window_Glass",(x,y,z),(.012,h,w),glass,.001)
    for side in (-1,1):cube(f"Window_Frame_Side_{side}",(x-.012,y,z+side*w*.5),(.05,h+.1,.045),frame,.012)
    for side in (-1,1):cube(f"Window_Frame_Horiz_{side}",(x-.012,y+side*h*.5,z),(.05,.045,w+.09),frame,.012)
    cube("Window_Mullion",(x-.02,y,z),(.06,h,.018),frame,.005)
    cube("City_Sky_Backdrop",(x+.45,y+.15,z),(.02,h+3,w+1.4),emissive("Night_City_Backdrop",(.008,.018,.055),.75),0)
    buildings=[material("City_Tower_Deep",(.012,.028,.072),.58,.28),material("City_Tower_Blue",(.025,.065,.14),.48,.32),material("City_Tower_Violet",(.055,.035,.12),.55,.2)]
    for i in range(17):
        bz=z-w*.63+i*(w*1.26/16);ht=2.2+((i*37)%8)*.43;dep=.35+((i*7)%4)*.14;bx=x+.30+((i*3)%4)*.16
        cube(f"City_Tower_{i}",(bx,.15+ht*.5,bz),(dep,ht,.32+(i%3)*.1),buildings[i%3],.012)
        for fl in range(max(1,int(ht/.32))):
            if (fl+i)%3==0:
                wm=emissive(f"City_Window_Light_{i}_{fl}",(.04,.25,.8) if i%2 else (.85,.32,.08),1.6)
                cube(f"City_Window_{i}_{fl}",(bx-dep*.51,.15+fl*.32+.18,bz),(.006,.10,.16),wm,.001)
    curtain=material("Curtain_Warm_Grey_Linen",(.32,.30,.29),.92)
    cube("Curtain_Rail",(x-.08,3.66,z),(.055,.065,w+.46),material("Curtain_Rail_Walnut",(.17,.09,.05),.3),.014)
    cw=max(.18,w*.18)
    for side in (-1,1):
        cz=z+side*(w*.5+cw*.36);nx,ny=34,22;vs=[];fs=[]
        for iy in range(ny+1):
            v=iy/ny
            for ix in range(nx+1):
                u=ix/nx;fold=math.sin(u*2*PI*8+side*.35)
                xx=x-.055-(.035*fold+.01*math.sin(u*2*PI*16));yy=y+(0.5-v)*h*1.18;zz=cz+(u-.5)*cw*.76
                vs.append((xx,yy,zz))
        for iy in range(ny):
            for ix in range(nx):
                a=iy*(nx+1)+ix;fs.append((a,a+1,a+nx+2,a+nx+1))
        me=bpy.data.meshes.new(f"Curtain_{side}_Mesh");me.from_pydata(vs,[],fs);me.materials.append(curtain);me.update()
        ob=bpy.data.objects.new(f"Right_Curtain_{side}",me);bpy.context.collection.objects.link(ob)
        so=ob.modifiers.new("Linen thickness","SOLIDIFY");so.thickness=.012
        su=ob.modifiers.new("Soft folds","SUBSURF");su.levels=1

def make_architecture():
    tr=material("Architectural_Black_Trim",(.018,.026,.052),.42,.15);hi=material("Architectural_Trim_Highlight",(.055,.075,.14),.34,.2)
    py,pz=2.48,-HD+.16
    for side in (-1,1):
        x=side*min(HW*.72,2.65);cube(f"Rear_Acoustic_Panel_{side}",(x,py,pz),(.42,1.08,.06),tr,.022)
        cube(f"Rear_Acoustic_Accent_{side}",(x,py,pz+.035),(.026,.88,.018),material(f"Acoustic_Accent_{side}",(.08,.16,.44),.28,.3),.008)
    for side in (-1,1):
        x=side*(HW-.105)
        cube(f"Side_Skirting_{side}",(x,.075,0),(.045,.15,HD*1.92),tr,.014)
        cube(f"Side_Ceiling_Trim_{side}",(x,RH-.13,0),(.035,.16,HD*1.9),tr,.016)
        for bay in range(5):cube(f"Side_Wall_Pilaster_{side}_{bay}",(x-side*.027,2.112,-HD*.78+bay*(HD*1.56/4)),(.012,2.552,.012),hi,.003)
    cube("Rear_Wall_Base_Moulding",(0,.56,-HD+.018),(HW*1.88,.025,.035),tr,.01)
    cube("Rear_Wall_Crown",(0,RH-.20,-HD+.018),(HW*1.88,.035,.045),tr,.014)
    for i in range(7):cube(f"Rear_Wall_Moulding_{i}",(-HW*.82+i*HW*1.64/6,.31,-HD+.012),(.018,.43,.025),hi,.005)
    for side in (-1,1):cube(f"Ceiling_Coffer_Rail_{side}",(side*HW*.46,RH-.035,0),(.018,.025,HD*1.72),tr,.006)
    for row in range(2):
        for col in range(3):
            x=(col-1)*HW*.62;z=(-1 if row==0 else 1)*HD*.42
            cube(f"Ceiling_Downlight_Bezel_{row}_{col}",(x,RH-.012,z),(.34,.012,.12),tr,.01)
            cube(f"Ceiling_Downlight_Core_{row}_{col}",(x,RH-.020,z),(.25,.006,.075),emissive(f"Downlight_Emission_{row}_{col}",(.22,.42,1) if row==0 else (.58,.20,1),1.2),.004)
    # Smoked-glass abstract art remains on the left/open wall.
    x,y,z=-HW+.085,2.52,1.75
    cube("Signal_Line_Art_Frame",(x,y,z),(.065,1.04,1.48),tr,.03)
    cube("Signal_Line_Art_Glass",(x-.037,y,z),(.012,.94,1.38),material("Signal_Art_Glass",(.035,.055,.105),.24,.32),.01)
    for i in range(3):
        yy=y+.28-i*.28;zz=z-.44+i*.19;e=emissive(f"Art_Trace_{i}",(.10,.72,1) if i!=1 else (.9,.06,.8),3)
        cube(f"Art_Trace_H_{i}",(x-.046,yy,zz),(.010,.018,.52),e,.001)
        cube(f"Art_Trace_V_{i}",(x-.046,yy-.08,zz+.24),(.010,.16,.018),e,.001)
        sphere(f"Art_Trace_Node_{i}",(x-.052,yy,zz-.25),(.012,.034,.034),e)

def make_pc_and_desk_detail():
    white=material("PC_Matte_White_Aluminium",(.82,.86,.92),.27,.12);edge=material("PC_White_Trim",(.94,.96,.98),.2,.18)
    glass=material("PC_Tempered_Glass",(.055,.075,.11),.12,.42);dark=material("PC_Internal_Graphite",(.13,.17,.22),.28,.7)
    x,y,z=DESK_X+.99,DESK_Y+.275,DESK_Z+.02
    cube("White_Gaming_PC_Tower",(x,y,z),(.25,.55,.43),white,.024,5)
    cube("PC_Tempered_Glass_Side",(x-.128,y,z),(.006,.48,.38),glass,.006)
    cube("PC_Front_Fascia",(x,y,z+.218),(.235,.52,.012),edge,.01)
    for i,col in enumerate(((.05,.65,1),(.55,.14,1),(.05,.9,.85))):
        cylinder(f"PC_Front_RGB_Fan_{i}",(x,y+.155-i*.15,z+.228),.115,.008,emissive(f"PC_Fan_Ring_{i}",col,2.8),48,.001,(PI/2,0,0))
    cube("PC_Motherboard",(x-.102,y+.01,z-.02),(.012,.31,.26),dark,.006)
    cube("PC_GPU",(x-.088,y-.115,z-.01),(.035,.055,.25),material("PC_GPU_Graphite",(.07,.09,.13),.24,.58),.006)
    ram=emissive("PC_RGB_RAM",(.14,.42,1),1.8)
    for i in range(2):cube(f"PC_RAM_{i}",(x-.095,y+.12,z-.12+i*.065),(.012,.16,.025),ram,.004)
    deskmat=material("Desk_Mat_Cloth",(.018,.025,.043),.93);kb=material("Keyboard_Aluminium",(.035,.045,.07),.3,.4);key=material("Keyboard_Keycaps",(.09,.11,.16),.6)
    cube("Desk_Mat",(DESK_X,DESK_Y+.014,DESK_Z+.30),(1.38,.018,.52),deskmat,.024)
    cube("Mechanical_Keyboard",(DESK_X-.12,DESK_Y+.032,DESK_Z+.27),(.48,.035,.17),kb,.014)
    for row in range(5):
        for col in range(15):
            x=DESK_X-.338+col*.031;z=DESK_Z+.201+row*.031
            km=emissive(f"Keyboard_RGB_{row}_{col}",(.04,.34,1),1) if row==0 and col in (2,6,10,13) else key
            cube(f"Keyboard_Key_{row}_{col}",(x,DESK_Y+.052,z),(.025,.012,.025),km,.003,2)
    sphere("Gaming_Mouse",(DESK_X+.39,DESK_Y+.045,DESK_Z+.28),(.055,.025,.085),material("Mouse_Matte",(.05,.065,.10),.27,.1))

def make_models(args):
    import_asset(args.bed,(BED_X,0,BED_Z),"Imported_Current_Bed")
    import_asset(args.desk,(DESK_X,0,DESK_Z),"Imported_User_Desk")
    screenmat=emissive("Monitor_PC_Stream",(.012,.025,.075),.75)
    # Actual supplied curved-monitor model. All three are loaded, outer screens hidden by default.
    for i,dx in enumerate((-.67,0,.67)):
        root,objs=import_asset(args.monitor,(DESK_X+dx,DESK_Y,MON_Z),f"Imported_Curved_Monitor_{i+1}")
        x=DESK_X-.67+i*.67
        overlay=curved_screen(f"Curved_Display_Surface_{i+1}",x,DESK_Y+.280,MON_Z+.020,.584,.328,.029,screenmat)
        w=overlay.matrix_world.copy();overlay.parent=root;overlay.matrix_world=w
        rig=(root,objs+[overlay]);MONITOR_RIGS.append(rig)
        if i!=1:
            root.hide_render=True
            for o in rig[1]:o.hide_render=True

def set_monitor_count(count):
    for i,(root,objs) in enumerate(MONITOR_RIGS):
        show=(count==3 or i==1)
        root.hide_render=not show
        for o in objs:o.hide_render=not show

CAMERAS=[
("01_room_entry_wide",(0,3.25,4.55),(0,1.3,-2.25),31,False),
("02_room_front_left",(-3.45,2.65,3.3),(.45,1.25,-2.15),32,False),
("03_room_front_right",(3.55,2.85,3.25),(0,1.25,-2.1),32,False),
("04_room_high_cutaway",(-3.3,6.8,2.9),(0,.85,-2.25),29,False),
("05_bed_three_quarter",(.05,3.35,.8),(BED_X,1.02,BED_Z-.1),42,False),
("06_headboard_close",(BED_X+.2,2.16,-1.85),(BED_X,1.36,BED_Z-1.53),58,True),
("07_headboard_reclined_view",(BED_X,.91,-1.8),(BED_X,.91,BED_Z-1.46),48,False),
("08_desk_corner",(-1.3,2.2,-3.42),(DESK_X,1,DESK_Z-.08),48,False),
("09_tv_and_media_console",(BED_X,2.15,1.25),(BED_X,1.18,TV_Z),52,False),
("10_window_and_curtains",(1.05,2.25,-3.25),(HW-.07,1.95,WIN_Z),47,False),
("11_left_bedside_detail",(1.55,1.55,-2.8),(BED_X-1.53,.72,BED_Z-.94),58,True),
("12_right_bedside_detail",(3.15,1.62,-2.6),(BED_X+1.53,.72,BED_Z-.94),58,True),
("13_entry_door_and_switch",(-.35,1.62,4.35),(.95,1.30,5.32),52,False),
("14_ceiling_cove_and_downlights",(-.15,3.55,1.3),(0,4.22,-3.20),48,False),
("15_floor_finish_and_inlays",(.45,.47,2.15),(1.65,.07,-1),64,True),
("16_plant_and_wall_art",(-2.2,1.85,2.3),(-4.10,1.55,1.8),55,False),
("17_floor_lamp",(-2.6,1.5,0),(-3.9,1.25,-1.45),58,True),
("18_headboard_led_channel_detail",(BED_X,1.93,-1.55),(BED_X,1.83,BED_Z-1.45),70,True),
("19_pc_tower_and_desk_accessories",(-1.75,1.55,-3.65),(DESK_X+.88,1.03,DESK_Z+.03),62,True)
]

def camera(name,loc,target,lens,dof):
    d=bpy.data.cameras.new(name);o=bpy.data.objects.new(name,d);bpy.context.collection.objects.link(o)
    o.location=loc;o.rotation_euler=(Vector(target)-o.location).to_track_quat("-Z","Y").to_euler();d.lens=lens;d.dof.use_dof=dof
    if dof:d.dof.focus_distance=(Vector(target)-o.location).length;d.dof.aperture_fstop=4.6
    return o

def render(scene,c,path):
    scene.camera=c;scene.render.filepath=str(path);bpy.ops.render.render(write_still=True)

def main():
    a=cli();out=Path(a.output_dir).resolve();rd=out/"renders";rd.mkdir(parents=True,exist_ok=True);reset()
    # Room shell and finish materials.
    wall=material("Room_Navy_Plaster",(.025,.035,.065),.82);floor=material("Polished_Midnight_Stone",(.055,.07,.11),.23,.20);ceil=material("Graphite_Ceiling",(.045,.055,.08),.70)
    cube("Room_Floor",(0,-.08,0),(HW*2+.1,.16,HD*2+.1),floor,.025,4)
    cube("Room_Ceiling",(0,RH+.05,0),(HW*2+.1,.10,HD*2+.1),ceil,.012)
    cube("Room_Left_Wall",(-HW-.05,RH/2,0),(.10,RH,HD*2),wall,.012)
    cube("Room_Back_Wall",(0,RH/2,-HD-.05),(HW*2,RH,.10),wall,.012)
    # Right wall is intentionally opened around the real glazing bay.
    zmin,zmax=WIN_Z-1.75,WIN_Z+1.75;wx=HW+.05
    cube("Right_Wall_Front_Return",(wx,RH/2,(zmax+HD)/2),(.10,RH,HD-zmax),wall,.012)
    cube("Right_Wall_Back_Return",(wx,RH/2,(-HD+zmin)/2),(.10,RH,zmin+HD),wall,.012)
    cube("Right_Wall_Sill_Band",(wx,.46,WIN_Z),(.10,.92,zmax-zmin),wall,.012)
    cube("Right_Wall_Header_Band",(wx,(3.08+RH)/2,WIN_Z),(.10,RH-3.08,zmax-zmin),wall,.012)
    sp=max(.25,(HW*2-1.4)*.5)
    cube("Entry_Left_Return",(-(HW+.7)*.5,RH/2,HD+.02),(sp,RH,.10),wall,.012)
    cube("Entry_Right_Return",((HW+.7)*.5,RH/2,HD+.02),(sp,RH,.10),wall,.012)
    cube("Entry_Transom",(0,RH-1.05,HD+.02),(1.4,2.1,.10),wall,.012)
    make_door();make_window();make_architecture();make_plant();make_floorlamp();make_models(a)
    make_pc_and_desk_detail();make_bedside();make_tv();add_leds()
    light("Main_Softbox",(-1.8,4.1,5),(.3,1,-2.1),(.77,.84,1),1700,5)
    light("Ceiling_Softbox",(0,4.16,-.6),(.2,.2,-1),(.60,.70,1),1250,4.2)
    light("Desk_Fill",(-3.6,2.8,-2.2),(-2.7,.8,-4.9),(.64,.76,1),520,2.2)
    light("Bed_Fill",(3.7,3.2,.2),(2.2,.8,-3.7),(.75,.78,1),680,3)
    light("Entry_Rim",(0,2.8,6.8),(0,1.5,2),(1,.72,.50),300,2.6)
    light("City_Spill",(4.55,2.1,-3.3),(3.2,1.5,-3.3),(.18,.34,1),120,2,2)
    w=bpy.data.worlds.new("Night_Penthouse_World");w.use_nodes=True;w.node_tree.nodes["Background"].inputs["Color"].default_value=(.018,.026,.06,1)
    w.node_tree.nodes["Background"].inputs["Strength"].default_value=.30;bpy.context.scene.world=w
    scene=bpy.context.scene;scene.render.engine="CYCLES";scene.cycles.device="CPU";scene.cycles.samples=16;scene.cycles.use_denoising=True
    scene.render.resolution_x=1600;scene.render.resolution_y=1000;scene.render.resolution_percentage=68
    scene.render.image_settings.file_format="PNG";scene.render.image_settings.color_mode="RGBA";scene.render.image_settings.compression=7
    scene.view_settings.view_transform="AgX"
    try:scene.view_settings.look="AgX - Medium High Contrast"
    except Exception:pass
    cams={};apply_theme(THEMES[0]);set_monitor_count(1)
    for name,loc,target,lens,dof in CAMERAS:
        c=camera(name,loc,target,lens,dof);cams[name]=c;render(scene,c,rd/(name+".png"))
    for theme in THEMES[1:]:
        apply_theme(theme)
        for nm in ("01_room_entry_wide","05_bed_three_quarter","08_desk_corner"):
            render(scene,cams[nm],rd/(theme[0]+"__"+nm+".png"))
    apply_theme(THEMES[0]);set_monitor_count(3)
    render(scene,cams["08_desk_corner"],rd/"20_desk_triple_monitor_option.png")
    set_monitor_count(1);scene.camera=cams["01_room_entry_wide"]
    bpy.ops.wm.save_as_mainfile(filepath=str(out/"room_showcase_scene.blend"))
    manifest={"source":"Current QuestBridge XR room renderer layout and current bed, desk and curved-monitor GLB assets",
      "room_size":"medium","dimensions_m":[HW*2,RH,HD*2],
      "anchors":{"bed":[BED_X,0,BED_Z],"desk":[DESK_X,0,DESK_Z],"tv":[BED_X,1.95,TV_Z]},
      "lighting_variants":[t[1] for t in THEMES],"images":sorted(p.name for p in rd.glob("*.png")),
      "notes":["Blender review renders from the current project assets; not captures from a running Quest.",
       "The headboard emitter is recessed behind projecting upholstered lips.",
       "LED colors are varied by independently recolouring the seven room zones.",
       "No bedside LED remote, chair or couch is added."]}
    (out/"manifest.json").write_text(json.dumps(manifest,indent=2),encoding="utf-8")
    print("ROOM_SHOWCASE_RENDER_COUNT",len(list(rd.glob("*.png"))),"OUTPUT",out)

if __name__=="__main__":main()
