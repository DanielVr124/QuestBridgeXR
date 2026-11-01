# QuestBridge XR — Brambley-reference headboard rebuild

The headboard is rebuilt around the visual structure of the Brambley Bed w/ LED Lights reference: a broad silver faux-leather insert with crystal-button diamond tufting, a layered beveled mirror frame, textured silver outer bands and a recessed perimeter LED channel.

## Headboard design
- Full-width silver-grey faux-leather upholstered insert with a softly crowned, furniture-like profile.
- Four staggered rows of clear polished crystal-style tuft buttons set into bright metal mounts, with diagonal seams forming a continuous diamond-tufted pattern.
- Polished mirror-chrome bevel rails and a soft grey recessed seam reveal around the upholstery.
- Direction-mapped alligator embossing on the silver outer bands, applied as continuous material texture rather than protruding tiles.
- One rounded, closed-loop LED raceway follows the complete outer edge; the strip is continuous around all four sides and corners.
- All five Blender review angles render with the blue-violet LED illuminated. This is a preview-only lighting override; the headset keeps its existing LED-zone toggle and straight-on visibility gate.
- The insert is one connected cushioned surface, not a row of vertical ribs or the previous undersized plain slab.

## Pillow refinement

The two main sleep pillows are built as editable named mesh parts rather than primitive ellipsoids. Their softly squared rounded-rectangle footprint keeps a clear pillow silhouette, while a restrained domed crown, continuous side gusset and closed underside give them real filling depth. A narrow welt and low-amplitude corner creases add fabric detail without excessive quilting or balloon-like puffiness. Their original centres, dimensions, propped angle and layering are preserved.

## What stays unchanged
- Original bed platform, rails, legs, foot end, mattress, fitted sheet, duvet and throw.
- Existing pillow centres and overall layout; the two main sleep pillows use separate softly crowned top shells, full-depth box gussets, undersides, narrow welt seams and restrained corner creases. The smaller accent and lumbar cushions retain their positions and receive gently eased profiles.
- Bed position, scale, renderer anchor and collision footprint.
- Bedside cabinets, lights, TV, desk, monitor, windows and all other room geometry.
- Camera transforms, head tracking, turning, locomotion, controller bindings and room menus.
- Existing LED color controls and the runtime's LED visibility logic.

## Regenerate the model
From the repository root:

```bash
QBX_BED_OUTPUT_DIR=art/bed \
QBX_BED_RUNTIME_GLB=quest-client/app/src/main/assets/models/arc_upholstered_bed.glb \
python tools/generate_bed_asset.py
```

With Blender 4.5.15 or newer:

```bash
blender --background --python tools/blender_import_bed.py -- \
  --source art/bed/arc_upholstered_bed_editable.glb \
  --output art/bed/arc_upholstered_bed.blend \
  --render-dir art/bed/renders
```

The Blender workflow generates the editable scene, runtime model, bed review renders and the room showcase package.
