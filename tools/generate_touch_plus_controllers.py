#!/usr/bin/env python3
"""Build high-detail, ring-free Meta Touch Plus visual replicas for QuestBridge XR.

The geometry is a clean Blender-authored reference recreation, not Meta's proprietary
source FBX. It uses the published Touch Plus silhouette, button layout and finish.
"""
from __future__ import annotations

import json
import math
from pathlib import Path

import bpy
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[1]
ASSET_DIR = ROOT / "quest-client/app/src/main/assets/models"
ART_DIR = ROOT / "art/controllers"
RENDER_DIR = ART_DIR / "renders"
ASSET_DIR.mkdir(parents=True, exist_ok=True)
RENDER_DIR.mkdir(parents=True, exist_ok=True)

# Clear the startup scene but retain a clean, editable Blender project.
bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
for collection in list(bpy.data.collections):
    if collection.name != "Collection":
        bpy.data.collections.remove(collection)

def material(name, color, metallic=0.0, roughness=0.42):
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.diffuse_color = (*color, 1.0)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    bsdf.inputs["Base Color"].default_value = (*color, 1.0)
    bsdf.inputs["Metallic"].default_value = metallic
    bsdf.inputs["Roughness"].default_value = roughness
    return mat

SHELL = material("TouchPlus | warm porcelain polymer", (0.79, 0.82, 0.84), 0.06, 0.31)
SHELL_LIGHT = material("TouchPlus | edge highlight polymer", (0.90, 0.92, 0.93), 0.04, 0.28)
FACE = material("TouchPlus | graphite faceplate", (0.030, 0.037, 0.046), 0.12, 0.30)
RUBBER = material("TouchPlus | soft grip elastomer", (0.060, 0.069, 0.078), 0.01, 0.68)
BUTTON = material("TouchPlus | button charcoal", (0.092, 0.105, 0.119), 0.04, 0.34)
BUTTON_EDGE = material("TouchPlus | button satin rim", (0.20, 0.22, 0.24), 0.28, 0.31)
GLYPH = material("TouchPlus | silk-screen markings", (0.73, 0.77, 0.80), 0.02, 0.45)
LED = material("TouchPlus | status indicator", (0.22, 0.75, 0.95), 0.04, 0.24)
METAL = material("TouchPlus | dark internal hardware", (0.17, 0.19, 0.21), 0.70, 0.28)

def active_collection(name):
    collection = bpy.data.collections.new(name)
    bpy.context.scene.collection.children.link(collection)
    layer = bpy.context.view_layer.layer_collection
    found = None
    def visit(node):
        nonlocal found
        if node.collection == collection:
            found = node
            return
        for child in node.children:
            visit(child)
    visit(layer)
    if found:
        bpy.context.view_layer.active_layer_collection = found
    return collection

def finish_mesh(obj, mat, bevel=0.0, smooth=True):
    obj.data.materials.clear()
    obj.data.materials.append(mat)
    if smooth and obj.type == "MESH":
        for face in obj.data.polygons:
            face.use_smooth = True
    if bevel > 0.0:
        mod = obj.modifiers.new("Soft manufactured radii", "BEVEL")
        mod.width = bevel
        mod.segments = 4
        mod.profile = 0.5
        mod.limit_method = "ANGLE"
        mod.angle_limit = math.radians(24)
        bpy.context.view_layer.objects.active = obj
        bpy.ops.object.modifier_apply(modifier=mod.name)
        normals = obj.modifiers.new("Weighted edge normals", "WEIGHTED_NORMAL")
        normals.keep_sharp = True
        bpy.ops.object.modifier_apply(modifier=normals.name)
    return obj

def rounded_box(name, location, dimensions, mat, bevel=0.003, rotation=None):
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=location)
    obj = bpy.context.object
    obj.name = name
    obj.dimensions = dimensions
    if rotation:
        obj.rotation_euler = rotation
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return finish_mesh(obj, mat, bevel, True)

def sphere(name, location, scale, mat, segments=24, rings=16):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=segments, ring_count=rings, radius=1.0, location=location)
    obj = bpy.context.object
    obj.name = name
    obj.scale = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return finish_mesh(obj, mat, 0.0, True)

def cylinder(name, location, radius, depth, mat, vertices=32, rotation=None, bevel=0.0008):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth, location=location)
    obj = bpy.context.object
    obj.name = name
    if rotation:
        obj.rotation_euler = rotation
    return finish_mesh(obj, mat, bevel, True)

def torus(name, location, major, minor, mat, rotation=None):
    bpy.ops.mesh.primitive_torus_add(major_radius=major, minor_radius=minor,
                                     major_segments=40, minor_segments=10, location=location)
    obj = bpy.context.object
    obj.name = name
    if rotation:
        obj.rotation_euler = rotation
    return finish_mesh(obj, mat, 0.0, True)

def label(name, body, location, size, mat, align="CENTER"):
    curve = bpy.data.curves.new(name, type="FONT")
    curve.body = body
    curve.size = size
    curve.align_x = align
    curve.align_y = "CENTER"
    curve.extrude = 0.00012
    curve.bevel_depth = 0.00004
    curve.bevel_resolution = 2
    obj = bpy.data.objects.new(name, curve)
    bpy.context.collection.objects.link(obj)
    obj.location = location
    # Printed legends face the controller's front (-Z).
    obj.rotation_euler = (math.pi, 0.0, 0.0)
    obj.data.materials.append(mat)
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.convert(target="MESH")
    return bpy.context.object

def build_controller(side):
    is_left = side == "left"
    prefix = "TouchPlus.L" if is_left else "TouchPlus.R"
    collection = active_collection(prefix)
    root = bpy.data.objects.new(prefix + ".Root", None)
    collection.objects.link(root)
    root.empty_display_type = "PLAIN_AXES"
    root.empty_display_size = 0.018
    created = [root]

    def register(obj):
        obj.name = prefix + "." + obj.name
        obj.parent = root
        created.append(obj)
        return obj

    def box(name, loc, dims, mat, bevel=0.003, rot=None):
        return register(rounded_box(name, loc, dims, mat, bevel, rot))

    def sph(name, loc, dims, mat):
        return register(sphere(name, loc, dims, mat))

    def cyl(name, loc, radius, depth, mat, vertices=32, rot=None):
        return register(cylinder(name, loc, radius, depth, mat, vertices, rot))

    def ring(name, loc, major, minor, mat, rot=None):
        return register(torus(name, loc, major, minor, mat, rot))

    def text(name, body, loc, size):
        return register(label(name, body, loc, size, GLYPH))

    # Ring-free tracked puck: rounded upper housing, inset graphite face and
    # a sculpted handle. The overall proportions are authored in metres.
    box("PuckShell", (0.0, 0.003, 0.002), (0.071, 0.024, 0.080), SHELL, 0.011)
    box("PuckShoulder", (0.0, -0.001, 0.010), (0.067, 0.018, 0.064), SHELL_LIGHT, 0.009)
    box("FaceplateInset", (0.0, -0.001, -0.014), (0.062, 0.010, 0.069), FACE, 0.010)
    box("FaceplateLowerSweep", (0.0, -0.010, -0.006), (0.053, 0.006, 0.030), BUTTON, 0.008)

    # Slightly swept palm grip and the moulded lower battery/strap housing.
    box("ErgonomicHandle", (0.0, -0.055, 0.021), (0.028, 0.084, 0.034),
        SHELL, 0.009, (math.radians(-9.0), 0.0, 0.0))
    box("RearGripPanel", (0.0, -0.060, 0.040), (0.023, 0.060, 0.004),
        RUBBER, 0.005, (math.radians(-9.0), 0.0, 0.0))
    box("BatteryDoorSeam", (0.0, -0.070, 0.043), (0.024, 0.0013, 0.004),
        METAL, 0.0005)
    for rib in range(6):
        y = -0.037 - rib * 0.009
        box("PalmGripRidge", (0.0, y, 0.043), (0.018, 0.0016, 0.0013),
            RUBBER, 0.0005)

    # Trigger blade and rear safety/grip switch are deliberately distinct.
    box("IndexTrigger", (0.0, 0.022, 0.046), (0.030, 0.009, 0.036),
        RUBBER, 0.004, (math.radians(-18.0), 0.0, 0.0))
    box("TriggerContact", (0.0, 0.026, 0.031), (0.022, 0.004, 0.021),
        BUTTON, 0.003)
    outer = -1.0 if is_left else 1.0
    box("GripPaddle", (outer * 0.037, -0.027, 0.023), (0.005, 0.034, 0.029),
        RUBBER, 0.002, (math.radians(-8.0), 0.0, 0.0))
    box("GripPaddleEdge", (outer * 0.0395, -0.026, 0.023), (0.0013, 0.024, 0.020),
        BUTTON_EDGE, 0.0006)

    # Tracking windows and small manufactured seams around the outer puck.
    for x, y, z in [(-0.029, 0.013, -0.005), (0.029, 0.013, -0.005),
                    (-0.030, -0.004, 0.020), (0.030, -0.004, 0.020)]:
        sph("OpticalWindow", (x, y, z), (0.0022, 0.0024, 0.0015), RUBBER)
    sph("StatusLED", (0.0, 0.016, -0.021), (0.0016, 0.0016, 0.0008), LED)
    box("PuckSideSeam", (outer * 0.0348, 0.003, 0.004), (0.0011, 0.014, 0.047),
        BUTTON_EDGE, 0.0004)

    # Analogue stick module, circular thumb cap and textured grip crown.
    stick_x = -0.017 if is_left else -0.017
    stick_y = 0.014
    cyl("ThumbstickBaseRing", (stick_x, stick_y, -0.0225), 0.0103, 0.0040, BUTTON_EDGE)
    cyl("ThumbstickSocket", (stick_x, stick_y, -0.0250), 0.0084, 0.0032, RUBBER)
    cyl("ThumbstickStem", (stick_x, stick_y, -0.0290), 0.0041, 0.0060, METAL)
    sph("ThumbstickCap", (stick_x, stick_y, -0.0325), (0.0081, 0.0045, 0.0081), BUTTON)
    sph("ThumbstickTouchInset", (stick_x, stick_y - 0.0010, -0.0390),
        (0.0031, 0.0008, 0.0031), RUBBER)

    if is_left:
        # Left hand: X/Y cluster and three-line menu button.
        buttons = [("X", -0.001, 0.018, "X"), ("Y", 0.018, 0.018, "Y")]
        menu_x = 0.026
        box("MenuButton", (menu_x, 0.026, -0.022), (0.010, 0.009, 0.0028), BUTTON, 0.003)
        for line in range(3):
            box("MenuGlyphLine", (menu_x, 0.027 + line * 0.0024, -0.0240),
                (0.0058, 0.0007, 0.0005), GLYPH, 0.0002)
    else:
        # Right hand: A/B cluster and Meta system/infinity mark.
        buttons = [("A", 0.018, 0.018, "A"), ("B", 0.018, -0.003, "B")]
        meta_x = 0.026
        box("MetaButton", (meta_x, 0.027, -0.022), (0.010, 0.009, 0.0028), BUTTON, 0.003)
        text("MetaInfinityMark", "∞", (meta_x, 0.027, -0.0240), 0.008)

    for name, x, y, glyph in buttons:
        cyl(name + "ButtonBezel", (x, y, -0.0220), 0.0058, 0.0025, BUTTON_EDGE)
        sph(name + "Button", (x, y, -0.0240), (0.0048, 0.0048, 0.0012), BUTTON)
        text(name + "Legend", glyph, (x, y, -0.0252), 0.0047)

    # Discreet function/pairing key and lanyard hardware under the heel.
    cyl("PairingKey", (outer * 0.025, -0.047, -0.014), 0.0036, 0.0020, BUTTON_EDGE)
    sph("PairingKeyInset", (outer * 0.025, -0.047, -0.0152), (0.0022, 0.0022, 0.0008), BUTTON)
    ring("WristStrapEyelet", (0.0, -0.098, 0.025), 0.0075, 0.0013, METAL,
         (math.radians(90.0), 0.0, 0.0))
    box("StrapLoopBridge", (0.0, -0.091, 0.025), (0.013, 0.008, 0.007), SHELL, 0.002)

    # Consolidate geometry per controller to keep the Quest draw-call count low.
    mesh_parts = [obj for obj in created[1:] if obj.type == "MESH"]
    bpy.ops.object.select_all(action="DESELECT")
    for obj in mesh_parts:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = mesh_parts[0]
    bpy.ops.object.join()
    combined = bpy.context.object
    combined.name = prefix + ".ControllerMesh"
    combined.parent = root
    created = [root, combined]
    bpy.context.view_layer.update()
    return root, created, collection

def export_glb(objects, path):
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
    bpy.ops.export_scene.gltf(filepath=str(path), export_format="GLB",
                              use_selection=True, export_apply=True,
                              export_yup=True, export_texcoords=True,
                              export_normals=True, export_materials="EXPORT",
                              export_cameras=False, export_lights=False)
    bpy.ops.object.select_all(action="DESELECT")

left_root, left_objects, left_collection = build_controller("left")
export_glb(left_objects, ASSET_DIR / "meta_touch_plus_left.glb")
right_root, right_objects, right_collection = build_controller("right")
export_glb(right_objects, ASSET_DIR / "meta_touch_plus_right.glb")

# Set the controllers side by side for the editable source file and gallery renders.
left_root.location.x = -0.075
right_root.location.x = 0.075
bpy.context.view_layer.update()

# Lighting and camera for actual Blender renders, not mock-up illustrations.
scene = bpy.context.scene
try:
    scene.render.engine = "BLENDER_EEVEE_NEXT"
except TypeError:
    # Ubuntu 22.04 repositories may package Blender 3.x.
    scene.render.engine = "BLENDER_EEVEE"
scene.render.resolution_x = 1400
scene.render.resolution_y = 1000
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = "PNG"
scene.render.image_settings.color_mode = "RGBA"
scene.render.film_transparent = False
try:
    scene.view_settings.view_transform = "AgX"
except TypeError:
    scene.view_settings.view_transform = "Filmic"
scene.world.color = (0.018, 0.022, 0.030)

def add_area(name, location, power, color, size):
    data = bpy.data.lights.new(name, type="AREA")
    data.energy = power
    data.color = color
    data.shape = "DISK"
    data.size = size
    obj = bpy.data.objects.new(name, data)
    scene.collection.objects.link(obj)
    obj.location = location
    obj.rotation_euler = (Vector((0, -0.035, 0)) - obj.location).to_track_quat("-Z", "Y").to_euler()
    return obj

def set_camera(name, location, target, ortho):
    data = bpy.data.cameras.new(name)
    camera = bpy.data.objects.new(name, data)
    scene.collection.objects.link(camera)
    camera.location = location
    camera.rotation_euler = (Vector(target) - camera.location).to_track_quat("-Z", "Y").to_euler()
    data.type = "ORTHO"
    data.ortho_scale = ortho
    scene.camera = camera
    return camera

add_area("Large softbox key", (0.28, 0.34, -0.30), 520, (0.78, 0.86, 1.0), 0.42)
add_area("Warm edge strip", (-0.38, 0.10, 0.18), 360, (1.0, 0.78, 0.56), 0.34)
add_area("Top fill", (0.02, 0.43, 0.16), 430, (0.85, 0.91, 1.0), 0.30)
add_area("Front bounce", (0.02, 0.10, -0.60), 190, (0.72, 0.84, 1.0), 0.26)

# Gallery view: front three-quarter, control-face detail, and true side profile.
gallery = [
    ("touch_plus_pair_three_quarter.png", (0.33, 0.20, -0.52), (0.0, -0.045, 0.008), 0.40),
    ("touch_plus_face_layout.png", (0.05, 0.10, -0.60), (0.0, -0.025, 0.000), 0.34),
    ("touch_plus_side_profile.png", (0.62, 0.02, -0.03), (0.0, -0.040, 0.014), 0.33),
]
for filename, camera_pos, target, scale in gallery:
    camera = set_camera("PreviewCamera", camera_pos, target, scale)
    scene.render.filepath = str(RENDER_DIR / filename)
    bpy.ops.render.render(write_still=True)
    bpy.data.objects.remove(camera, do_unlink=True)

# Store a manifest with dimensions, source provenance, and generated deliverables.
manifest = {
    "model": "Meta Quest 3 Touch Plus",
    "type": "high-detail Blender reference recreation",
    "provenance": "Authored in Blender from Meta Touch Plus public product/reference imagery; not Meta's proprietary FBX source mesh.",
    "dimensions_m": {"body_width": 0.071, "head_height": 0.024, "grip_length": 0.084},
    "left_glb": "quest-client/app/src/main/assets/models/meta_touch_plus_left.glb",
    "right_glb": "quest-client/app/src/main/assets/models/meta_touch_plus_right.glb",
    "editable_source": "art/controllers/meta_touch_plus_controllers.blend",
    "renders": [p.name for p in sorted(RENDER_DIR.glob("*.png"))],
}
(ART_DIR / "controller_manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
bpy.ops.wm.save_as_mainfile(filepath=str(ART_DIR / "meta_touch_plus_controllers.blend"))
print("Touch Plus controller GLBs, Blender source, and renders generated.")
