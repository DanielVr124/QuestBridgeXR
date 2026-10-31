#!/usr/bin/env python3
"""Build QuestBridgeXR's replacement upholstered bed GLB with named, editable parts.

Coordinate convention matches the native Quest room: X=width, Y=up, Z=bed length.
The headboard is at -Z. All dimensions are metres, with the underside feet near Y=0.
"""
from __future__ import annotations
import math, os, json
from pathlib import Path
import numpy as np
from PIL import Image
import trimesh
from trimesh.visual.material import PBRMaterial
from trimesh.visual.texture import TextureVisuals

REPO_ROOT = Path(__file__).resolve().parents[2]
OUT = REPO_ROOT / 'art' / 'bed' / 'generated'
OUT.mkdir(parents=True, exist_ok=True)
TEXTURES = OUT / 'textures'
TEXTURES.mkdir(exist_ok=True)

def make_weave(name, base, seed, mode='linen', size=768):
    """Create a seamless, low-contrast woven albedo tile for realtime use."""
    rr = np.random.default_rng(seed)
    y, x = np.mgrid[0:size, 0:size].astype(np.float32)
    xn=x/size; yn=y/size
    noise=rr.normal(0,1,(size,size)).astype(np.float32)
    warp=np.sin(2*np.pi*(x*0.205 + 0.06*np.sin(2*np.pi*yn*3.0)))
    weft=np.sin(2*np.pi*(y*0.205 + 0.05*np.sin(2*np.pi*xn*4.0)))
    fine=np.sin(2*np.pi*(x*0.82 + y*0.13))*0.35 + np.sin(2*np.pi*(y*0.79-x*0.09))*0.32
    if mode=='quilt':
        diag=np.sin(2*np.pi*(xn*11.0+yn*11.0)) * np.sin(2*np.pi*(xn*11.0-yn*11.0))
        texture=warp*1.15+weft*.95+fine*.65+noise*.70+diag*.55
    elif mode=='rib':
        texture=warp*1.50+weft*.45+fine*.55+noise*.65
    else:
        texture=warp*1.0+weft*.90+fine*.55+noise*.75
    texture=np.clip(texture,-3.5,3.5)
    arr=np.stack([np.clip(base[c] + texture*(1.0 if c==0 else .93 if c==1 else .88),0,255) for c in range(3)],axis=-1).astype(np.uint8)
    img=Image.fromarray(arr,'RGB')
    img.save(TEXTURES/f'{name}.png',optimize=True)
    return img

textures={
 'charcoal':make_weave('upholstery_charcoal',(46,50,61),101,'linen'),
 'charcoal2':make_weave('upholstery_shadow',(35,39,49),103,'linen'),
 'linen':make_weave('linen_silver',(181,185,194),107,'linen'),
 'sheet':make_weave('cotton_sheet',(202,205,210),109,'linen'),
 'duvet':make_weave('duvet_graphite',(52,59,76),113,'quilt'),
 'throw':make_weave('throw_midnight',(37,49,82),127,'rib'),
 'pillow':make_weave('accent_pillow',(94,101,119),131,'linen'),
 'underside':make_weave('underside_dark',(24,29,40),137,'linen'),
}
materials={}
material_names={
 'charcoal':'Charcoal woven upholstery', 'charcoal2':'Shadow charcoal upholstery',
 'linen':'Silver-grey linen', 'sheet':'Soft cotton percale', 'duvet':'Graphite quilted duvet',
 'throw':'Midnight blue ribbed throw', 'pillow':'Slate accent cushion', 'underside':'Dark underframe textile'
}
for key,img in textures.items():
    materials[key]=PBRMaterial(name=material_names[key],baseColorFactor=[255,255,255,255],
        metallicFactor=0.0,roughnessFactor=0.92,baseColorTexture=img,doubleSided=True)

scene=trimesh.Scene(base_frame='BedRoot')
meshes=[]

def add_mesh(name,vertices,faces,uv,matkey,smooth=True):
    m=trimesh.Trimesh(vertices=np.asarray(vertices,dtype=np.float32),
        faces=np.asarray(faces,dtype=np.int64),process=False)
    m.visual=TextureVisuals(uv=np.asarray(uv,dtype=np.float32),material=materials[matkey])
    if smooth:
        m.merge_vertices();m.remove_unreferenced_vertices();m.fix_normals()
    scene.add_geometry(m,geom_name=name,node_name=name)
    meshes.append((name,m,matkey))
    return m

def spow(v,e): return np.sign(v)*np.abs(v)**e

def superellipsoid(name,center,radii,matkey='charcoal',exponent=.52,nu=64,nv=36,taper=None):
    """Rounded padded form with continuous UV mapping."""
    cx,cy,cz=center;rx,ry,rz=radii
    verts=[];uvs=[]
    for j in range(nv+1):
        lat=-math.pi/2+math.pi*j/nv
        cv=spow(math.cos(lat),exponent);sy=spow(math.sin(lat),exponent)
        for i in range(nu+1):
            lon=-math.pi+2*math.pi*i/nu
            px=spow(math.cos(lon),exponent)*cv
            pz=spow(math.sin(lon),exponent)*cv
            py=sy
            if taper is not None:
                amount=taper*abs(py);px*=1.0-amount*.24;pz*=1.0-amount*.18
            verts.append((cx+rx*px,cy+ry*py,cz+rz*pz));uvs.append((i/nu,j/nv))
    faces=[]
    for j in range(nv):
        for i in range(nu):
            a=j*(nu+1)+i;b=a+1;d=(j+1)*(nu+1)+i;c=d+1
            faces.extend(((a,b,c),(a,c,d)))
    return add_mesh(name,verts,faces,uvs,matkey)

def boxsoft(name,center,dims,matkey='charcoal',exponent=.44,**kw):
    return superellipsoid(name,center,(dims[0]/2,dims[1]/2,dims[2]/2),matkey,exponent,**kw)

def curve_tube(name,points,radius,matkey='charcoal2',sides=7):
    """One continuous, smoothly transported tube mesh along a polyline."""
    pts=np.asarray(points,dtype=float)
    if len(pts)<2:return
    tangents=[]
    for i in range(len(pts)):
        if i==0:t=pts[1]-pts[0]
        elif i==len(pts)-1:t=pts[-1]-pts[-2]
        else:t=pts[i+1]-pts[i-1]
        t=t/max(np.linalg.norm(t),1e-9);tangents.append(t)
    tangents=np.asarray(tangents)
    helper=np.array([0.,1.,0.]) if abs(tangents[0][1])<.92 else np.array([1.,0.,0.])
    n=np.cross(tangents[0],helper);n/=max(np.linalg.norm(n),1e-9)
    verts=[];uv=[]
    for j,(point,tangent) in enumerate(zip(pts,tangents)):
        if j:
            axis=np.cross(tangents[j-1],tangent);axis_len=np.linalg.norm(axis)
            if axis_len>1e-8:
                axis/=axis_len;angle=math.atan2(axis_len,float(np.dot(tangents[j-1],tangent)))
                n=n*math.cos(angle)+np.cross(axis,n)*math.sin(angle)+axis*np.dot(axis,n)*(1-math.cos(angle))
        n-=tangent*np.dot(n,tangent);n/=max(np.linalg.norm(n),1e-9)
        b=np.cross(tangent,n);b/=max(np.linalg.norm(b),1e-9)
        for k in range(sides+1):
            a=2*math.pi*k/sides
            verts.append(point+radius*(n*math.cos(a)+b*math.sin(a)))
            uv.append((k/sides,j/max(1,len(pts)-1)))
    faces=[];stride=sides+1
    for j in range(len(pts)-1):
        for k in range(sides):
            a=j*stride+k;b=a+1;d=(j+1)*stride+k;c=d+1
            faces.extend(((a,b,c),(a,c,d)))
    return add_mesh(name,verts,faces,uv,matkey)

def surface_grid(name,x_range,z_range,nx,nz,height_fn,matkey,thickness=.025):
    """A gently folded and closed double-sided sheet surface."""
    xs=np.linspace(x_range[0],x_range[1],nx+1);zs=np.linspace(z_range[0],z_range[1],nz+1)
    verts=[];uvs=[]
    for j,z in enumerate(zs):
        for i,x in enumerate(xs):
            verts.append((x,height_fn(x,z),z));uvs.append((i/nx,j/nz))
    n_top=len(verts)
    for j,z in enumerate(zs):
        for i,x in enumerate(xs):
            verts.append((x,height_fn(x,z)-thickness,z));uvs.append((i/nx,j/nz))
    faces=[];stride=nx+1
    for j in range(nz):
        for i in range(nx):
            a=j*stride+i;b=a+1;d=(j+1)*stride+i;c=d+1
            faces.extend(((a,b,c),(a,c,d)))
            ab=n_top+a;bb=n_top+b;cb=n_top+c;db=n_top+d
            faces.extend(((ab,cb,bb),(ab,db,cb)))
    perimeter=list(range(stride))
    perimeter += [j*stride+nx for j in range(1,nz+1)]
    perimeter += [nz*stride+i for i in range(nx-1,-1,-1)]
    perimeter += [j*stride for j in range(nz-1,0,-1)]
    for k,a in enumerate(perimeter):
        b=perimeter[(k+1)%len(perimeter)]
        faces.extend(((a,n_top+a,n_top+b),(a,n_top+b,b)))
    return add_mesh(name,verts,faces,uvs,matkey)

# ----------------------- FRAME AND SUSPENSION -----------------------
# Six short, inset feet, a floating upholstered plinth and padded perimeter rails.
for ix,x in enumerate((-1.02,1.02)):
    for iz,z in enumerate((-1.38,.12,1.39)):
        superellipsoid(f'Foot_{ix}_{iz}_satin_black',(x,.105,z),(.075,.105,.075),'underside',.42,32,20)
        superellipsoid(f'FootPad_{ix}_{iz}',(x,.024,z),(.060,.018,.060),'charcoal2',.60,24,14)
boxsoft('Underframe_shadow',(0,.235,0),(2.35,.27,3.08),'underside',.46)
boxsoft('Upholstered_platform',(0,.405,0),(2.48,.43,3.20),'charcoal',.48,nu=72,nv=40)
for side in (-1,1):
    superellipsoid(f'Side_rail_{"L" if side<0 else "R"}',(side*1.135,.435,.02),(.145,.255,1.56),'charcoal',.48,72,40)
superellipsoid('Footboard',(0,.445,1.49),(1.205,.245,.155),'charcoal',.50,72,40)
for side in (-1,1):
    x=side*1.135
    curve_tube(f'Side_rail_welt_{side}',[(x,.626,z) for z in np.linspace(-1.40,1.35,32)],.006,'charcoal2')
curve_tube('Footboard_welt',[(x,.625,1.49) for x in np.linspace(-1.08,1.08,36)],.006,'charcoal2')

# ----------------------- MATTRESS AND BEDDING -----------------------
superellipsoid('Mattress_core',(0,.720,.00),(1.115,.205,1.485),'linen',.55,88,48)
curve_tube('Mattress_lower_welt',[(-1.04,.635,z) for z in np.linspace(-1.28,1.28,36)],.004,'charcoal2')
curve_tube('Mattress_foot_welt',[(x,.635,1.42) for x in np.linspace(-1.0,1.0,32)],.004,'charcoal2')
def fitted_sheet_y(x,z):
    return .916+.008*math.sin(6*z+2*x)*math.cos(5*x)-.018*max(0,(abs(x)-.92)/.18)
surface_grid('Fitted_cotton_sheet',(-1.08,1.08),(-1.50,-.84),64,30,fitted_sheet_y,'sheet',.018)

# Graphite duvet: low-amplitude creases and relaxed, weighted folds.
def duvet_height(x,z):
    base=.968
    waves=(.012*math.sin(7.4*x+3.2*z)+.008*math.sin(13.8*z-4.4*x)+.005*math.sin(22*z+9*x))
    fold1=.026*math.exp(-((x+.52+.19*(z-.1))/.15)**2)*math.exp(-((z-.15)/1.7)**4)
    fold2=.021*math.exp(-((x-.62-.12*(z-.4))/.12)**2)*math.exp(-((z-.25)/1.7)**4)
    fold3=.025*math.exp(-((z-1.20+.18*x)/.16)**2)*(0.65+0.35*math.cos(x*2.5))
    side=max(0.,(abs(x)-.93)/.24);foot=max(0.,(z-1.28)/.50);head=max(0.,(-z-.93)/.42)
    return base+waves+fold1+fold2+fold3-.20*side*side-.30*foot*foot-.07*head*head
surface_grid('Graphite_duvet_with_soft_folds',(-1.19,1.19),(-1.16,1.78),96,128,duvet_height,'duvet',.035)
for side in (-1,1):
    curve_tube(f'Duvet_side_piping_{side}',[(side*1.19,duvet_height(side*1.19,z)-.006,z) for z in np.linspace(-1.12,1.73,54)],.004,'linen')
curve_tube('Duvet_foot_piping',[(x,duvet_height(x,1.78)-.006,1.78) for x in np.linspace(-1.16,1.16,48)],.004,'linen')

def throw_height(x,z):
    return duvet_height(x,z)+.028+.010*math.sin(x*8.5+z*5.2)+.006*math.sin(z*21-x*3)
surface_grid('Midnight_blue_ribbed_throw',(-1.16,1.16),(.88,1.69),76,44,throw_height,'throw',.025)
def throw_drop_height(x,t):
    return throw_height(x,1.69)-(.43*t)+.012*math.sin(x*7+t*5)
verts=[];uv=[];nx=64;nt=18
for j in range(nt+1):
    t=j/nt
    for i in range(nx+1):
        x=-1.15+2.30*i/nx
        verts.append((x,throw_drop_height(x,t),1.69+.025*t));uv.append((i/nx,t))
faces=[]
for j in range(nt):
    for i in range(nx):
        a=j*(nx+1)+i;b=a+1;d=(j+1)*(nx+1)+i;c=d+1
        faces.extend(((a,b,c),(a,c,d)))
add_mesh('Throw_foot_drape',verts,faces,uv,'throw')
curve_tube('Throw_hem',[(x,throw_drop_height(x,1)-.004,1.715) for x in np.linspace(-1.15,1.15,54)],.009,'pillow')

# ----------------------- PILLOWS AND HEADBOARD -----------------------
for side in (-1,1):
    x=side*.49
    superellipsoid(f'Sleep_pillow_{side}',(x,1.095,-1.115),(.52,.175,.335),'sheet',.48,72,44,taper=.34)
    curve_tube(f'Sleep_pillow_piping_{side}',[(x+u,1.19,-1.405+.025*math.cos(u*2.2)) for u in np.linspace(-.43,.43,28)],.0038,'linen')
for side in (-1,1):
    x=side*.285
    superellipsoid(f'Slate_accent_cushion_{side}',(x,1.215,-.795),(.37,.155,.245),'pillow',.50,64,40,taper=.26)
superellipsoid('Lumbar_cushion',(0,1.245,-.57),(.46,.135,.155),'throw',.53,64,36,taper=.30)

superellipsoid('Headboard_back_shell',(0,1.185,-1.575),(1.29,.925,.115),'charcoal2',.50,88,52)
superellipsoid('Headboard_top_rail',(0,2.035,-1.475),(1.245,.085,.125),'charcoal',.55,72,36)
for i in range(7):
    x=(i-3)*.354
    superellipsoid(f'Headboard_channel_{i+1:02d}',(x,1.275,-1.435),(.174,.735,.112),'charcoal',.50,64,44,taper=.18)
    curve_tube(f'Headboard_channel_stitch_{i+1:02d}',[(x,.72+j*.043,-1.326+.006*math.cos(j*.18)) for j in range(29)],.0018,'linen')
for side in (-1,1):
    superellipsoid(f'Headboard_side_bolster_{side}',(side*1.205,1.255,-1.415),(.105,.77,.135),'charcoal2',.50,56,36)
    boxsoft(f'Headboard_mount_{side}',(side*.92,.74,-1.53),(.105,.48,.13),'underside',.40)

scene.metadata.update({
 'asset':'QuestBridgeXR upholstered platform bed','units':'metres',
 'axes':'X width, Y up, Z length; headboard at -Z',
 'design':'Charcoal channel-upholstered headboard, padded upholstered rails, silver cotton bedding, graphite duvet, midnight ribbed throw',
 'references':['Walmart charcoal upholstered platform bed','Omax Decor Exton vertical-channel bed','Wayfair charcoal upholstered platform bed'],
 'generated_by':'art/bed/generate_bed_asset.py','triangle_budget':'optimized for mobile room renderer'
})
editable_path=OUT/'arc_upholstered_bed_editable.glb'
editable_path.write_bytes(scene.export(file_type='glb'))

runtime_scene=trimesh.Scene(base_frame='BedRoot')
for key in materials:
    group=[m for _name,m,material_key in meshes if material_key==key]
    if not group:continue
    vertices=[];faces=[];uvs=[];offset=0
    for part in group:
        vertices.append(np.asarray(part.vertices,dtype=np.float32))
        faces.append(np.asarray(part.faces,dtype=np.int64)+offset)
        uvs.append(np.asarray(part.visual.uv,dtype=np.float32))
        offset+=len(part.vertices)
    merged=trimesh.Trimesh(vertices=np.vstack(vertices),faces=np.vstack(faces),process=False)
    merged.visual=TextureVisuals(uv=np.vstack(uvs),material=materials[key])
    merged.fix_normals()
    runtime_scene.add_geometry(merged,geom_name=f'Bed_{key}',node_name=f'Bed_{key}')
runtime_scene.metadata.update(scene.metadata)
glb_path=OUT/'arc_upholstered_bed.glb'
glb_path.write_bytes(runtime_scene.export(file_type='glb'))
bounds=scene.bounds;extents=bounds[1]-bounds[0]
manifest={
 'runtime_glb':glb_path.name,'runtime_draw_meshes':len(runtime_scene.geometry),
 'editable_glb':editable_path.name,'editable_mesh_objects':len(meshes),
 'triangles':int(sum(len(m.faces) for _,m,_ in meshes)),
 'bounds_min':bounds[0].round(4).tolist(),'bounds_max':bounds[1].round(4).tolist(),
 'dimensions':extents.round(4).tolist(),'materials':list(materials),
 'objects':[{'name':n,'material':k,'triangles':len(m.faces)} for n,m,k in meshes]
}
(OUT/'bed_manifest.json').write_text(json.dumps(manifest,indent=2))
print(json.dumps(manifest,indent=2))
print('WROTE runtime',glb_path,glb_path.stat().st_size)
print('WROTE editable',editable_path,editable_path.stat().st_size)
