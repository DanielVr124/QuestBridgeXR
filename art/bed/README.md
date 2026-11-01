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

Exactly two full-size sleep pillows remain on the bed, one on each side with a small gap between them. Both are propped back against the headboard at a steep, natural recline: the lower front edge rests on the mattress while the upper rear edge leans into the upholstery instead of lying flat. Each pillow is an editable rounded-rectangle mesh with a fuller, softly domed crown, a slightly squarer rounded silhouette, a deep continuous box gusset and a closed underside. The crown has a tiny amount of uneven fill and shallow corner tucks sculpted into the mesh rather than artificial raised crease-lines. The shell uses a warm ivory cotton-percale texture, with a subtly darker matching gusset and very fine low-contrast seam thread. A denser, smoother mesh keeps the soft transitions clean without adding decorative quilting. There are no accent cushions or lumbar pillow.

## What stays unchanged
- Original bed platform, rails, legs, foot end, mattress, fitted sheet, duvet and throw.
- Original bed position, platform, bedding, headboard and room layout. Exactly two sleep pillows are present; both are propped against the headboard, with no extra accent or lumbar cushions.
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
