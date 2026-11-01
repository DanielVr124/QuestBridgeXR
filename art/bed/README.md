# QuestBridge XR — Brambley-reference headboard rebuild

The headboard is rebuilt around the visual structure of the Brambley Bed w/ LED Lights reference: a broad silver faux-leather insert with crystal-button diamond tufting, a layered beveled mirror frame, textured silver outer bands and a recessed perimeter LED channel.

## Headboard design
- Full-width silver-grey faux-leather upholstered insert with a softly crowned, furniture-like profile.
- Four rows of crystal-style tuft buttons set into small shadowed mounts, with diagonal seams forming the diamond-tufted pattern.
- Separate silver beveled frame rails and a fine dark reveal around the upholstery.
- Subtle raised alligator-style embossing across the outer silver bands.
- LED channel recessed behind the frame and aligned to the existing headboard light perimeter; the compositor's existing reclined-view visibility behavior is preserved.
- The insert is one connected cushioned surface, not a row of vertical ribs or the previous undersized plain slab.

## What stays unchanged
- Original bed platform, rails, legs, foot end, mattress, fitted sheet, duvet and throw.
- Existing refreshed pillow models and their placement.
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
