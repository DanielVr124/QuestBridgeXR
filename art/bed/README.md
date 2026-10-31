# QuestBridgeXR upholstered bed asset

This is a custom mesh assembly replacing the old hand-authored bed primitives.
Its world anchor and room collision footprint are intentionally unchanged.

## Generated files

The reproducible generator writes to art/bed/generated/:

- arc_upholstered_bed.glb — optimized Quest runtime model: eight material meshes, embedded fabric maps, about 230k triangles.
- arc_upholstered_bed_editable.glb — 58 separately named mesh objects for editing.
- textures/ — eight seamless fabric albedo tiles.
- bed_manifest.json — dimensions, materials, mesh inventory and triangle counts.

The Quest Android build copies the optimized GLB to:
quest-client/app/src/main/assets/models/arc_upholstered_bed.glb.

## Blender project

tools/blender_import_bed.py imports the separately named mesh components and packed fabric textures into a clean Blender scene, organizes the bed and studio into separate collections, configures a softbox lighting rig, saves art/bed/arc_upholstered_bed.blend, and renders art/bed/arc_upholstered_bed_blender_render.png.

Use Blender 4.5 LTS or later:

    python3 art/bed/generate_bed_asset.py

    blender --background --python tools/blender_import_bed.py -- --source art/bed/generated/arc_upholstered_bed_editable.glb

## Design direction and references

The silhouette uses a charcoal upholstered platform with a tall seven-channel padded headboard, soft upholstered rails, dark inset feet, a light cotton fitted sheet, graphite duvet, layered pillows, and a midnight-blue ribbed throw. The material maps are low-contrast and use high roughness to read as fabric in the Quest room.

Reference imagery:
- Walmart charcoal upholstered king platform bed: https://www.walmart.com/ip/5367366086
- Omax Decor Exton vertical-channel upholstered bed: https://www.omaxdecor.com/products/exton-upholstered-platform-king-bed-dark-gray
- Wayfair charcoal upholstered platform bed: https://www.wayfair.com/furniture/pdp/latitude-run-tufted-upholstered-platform-bed-w118634062.html
