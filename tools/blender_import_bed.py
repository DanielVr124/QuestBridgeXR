"""Create a fully editable Blender project and a lit studio render for the bed.

Run from the repository root:
  blender --background --python tools/blender_import_bed.py -- \
    --source art/bed/generated/arc_upholstered_bed_editable.glb

The source GLB keeps each component as a separate named mesh. The Quest app
uses a distinct merged eight-material GLB to keep draw calls low.
"""
from __future__ import annotations
import argparse
import sys
from pathlib import Path
import bpy
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'art' / 'bed' / 'generated' / 'arc_upholstered_bed_editable.glb'
OUTPUT = ROOT / 'art' / 'bed' / 'arc_upholstered_bed.blend'
RENDER = ROOT / 'art' / 'bed' / 'arc_upholstered_bed_blender_render.png'

args_list = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, default=SOURCE)
parser.add_argument('--output', type=Path, default=OUTPUT)
parser.add_argument('--render', type=Path, default=RENDER)
args = parser.parse_args(args_list)
if not args.source.exists():
    raise FileNotFoundError(f'Editable bed GLB not found: {args.source}')

# Clear the default scene.
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
for collection in list(bpy.data.collections):
    if collection.name != 'Collection':
        bpy.data.collections.remove(collection)

root = bpy.context.scene.collection
model_collection = bpy.data.collections.new('BED MODEL | editable named parts')
studio_collection = bpy.data.collections.new('STUDIO | render-only setup')
root.children.link(model_collection)
root.children.link(studio_collection)

# Import the generated source meshes with their packed fabric textures.
bpy.ops.import_scene.gltf(filepath=str(args.source))
for obj in list(bpy.context.scene.objects):
    if obj.type == 'MESH':
        for collection in list(obj.users_collection):
            collection.objects.unlink(obj)
        model_collection.objects.link(obj)
        obj.name = obj.name.replace('Bed_', '').replace('Headboard_', 'Headboard | ')
        obj['source_asset'] = 'QuestBridgeXR upholstered platform bed'
        if obj.data and obj.data.uv_layers:
            obj.data.uv_layers.active.name = 'Fabric UVs'
        for polygon in obj.data.polygons:
            polygon.use_smooth = True

# Render-only studio floor and softboxes. These live in their own collection.
def move_to_collection(obj, collection):
    for existing in list(obj.users_collection):
        existing.objects.unlink(obj)
    collection.objects.link(obj)
    return obj

bpy.ops.mesh.primitive_plane_add(size=200, location=(0, -0.035, 0))
floor = bpy.context.object
floor.name = 'Studio floor (render only)'
move_to_collection(floor, studio_collection)
floor_mat = bpy.data.materials.new('Studio | midnight slate')
floor_mat.diffuse_color = (.012, .018, .034, 1)
floor_mat.use_nodes = True
floor_bsdf = floor_mat.node_tree.nodes.get('Principled BSDF')
floor_bsdf.inputs['Base Color'].default_value = (.012, .018, .034, 1)
floor_bsdf.inputs['Roughness'].default_value = .84
floor.data.materials.append(floor_mat)

def aim(obj, point):
    obj.rotation_euler = (Vector(point) - obj.location).to_track_quat('-Z', 'Y').to_euler()

def area(name, location, power, color, size, target=(0, .9, 0)):
    light_data = bpy.data.lights.new(name, 'AREA')
    light_data.energy = power
    light_data.color = color
    light_data.shape = 'DISK'
    light_data.size = size
    obj = bpy.data.objects.new(name, light_data)
    studio_collection.objects.link(obj)
    obj.location = location
    aim(obj, target)
    return obj

area('Key | large softbox', (3.8, 5.2, 4.1), 620, (.78, .84, 1.0), 4.2)
area('Fill | cool fabric reveal', (-4.0, 3.0, 1.0), 420, (.42, .55, 1.0), 4.8)
area('Rim | headboard separation', (0.0, 4.1, -4.4), 500, (.60, .34, 1.0), 3.0)
area('Foot | linen detail', (1.0, 2.5, 5.0), 250, (1.0, .78, .70), 2.6)

camera_data = bpy.data.cameras.new('Bed studio camera')
camera = bpy.data.objects.new('Bed studio camera', camera_data)
studio_collection.objects.link(camera)
camera.location = (4.4, 3.1, 4.0)
aim(camera, (0, .95, 0))
camera_data.lens = 52

scene = bpy.context.scene
scene.camera = camera
scene.render.engine = 'CYCLES'
scene.cycles.device = 'CPU'
scene.cycles.samples = 48
scene.render.resolution_x = 1800
scene.render.resolution_y = 1350
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = 'PNG'
scene.render.filepath = str(args.render)
scene.world.color = (.018, .022, .040)
scene.view_settings.view_transform = 'AgX'
scene.render.film_transparent = False
scene.camera.data.dof.use_dof = False
scene.render.image_settings.color_mode = 'RGBA'
scene['asset_notes'] = (
    'Charcoal seven-channel upholstered headboard, padded wraparound rails, '
    'fitted cotton sheet, folded graphite duvet and midnight ribbed throw. '
    'Dimensions are metres. Model and studio collections are separated.'
)

# Leave the selectable bed mesh active in the saved project.
bpy.ops.object.select_all(action='DESELECT')
for obj in model_collection.objects:
    obj.select_set(True)
if model_collection.objects:
    bpy.context.view_layer.objects.active = model_collection.objects[0]

args.output.parent.mkdir(parents=True, exist_ok=True)
args.render.parent.mkdir(parents=True, exist_ok=True)
bpy.ops.wm.save_as_mainfile(filepath=str(args.output))
bpy.ops.render.render(write_still=True)
print(f'Saved Blender project: {args.output}')
print(f'Rendered studio preview: {args.render}')
