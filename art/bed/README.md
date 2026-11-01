# QuestBridge XR — original bed restoration with a targeted headboard + pillow update

This update restores the original generated bed body and changes **only the headboard upholstery and pillows**.

## Headboard
- Five broad, softly filled charcoal panels.
- Rounded/eased edges are deliberately restrained: the sections stay rectangular and do not become oversized oval cushions.
- A nested upholstered perimeter creates a narrow recessed RGB light channel.
- The strip remains hidden from a reclined/lying-down view and only reveals when the headset is aimed nearly straight at the headboard.
- No exposed LED tape, added footboard, diamond buttons or decorative crystal parts.

## Pillows
- The original pillow arrangement and positions are retained: two sleeping pillows, two smaller accent cushions and one lumbar cushion.
- Their soft forms, fabric welt/seams and subtle corner tension folds are refreshed to make them read as filled fabric rather than hard blocks.

## What stays exactly as it was
- Original bed platform, rails, legs and foot end.
- Original mattress, fitted sheet, duvet and throw.
- Bed position, scale, renderer anchor and collision footprint.
- Bedside cabinets, lights, TV, desk, monitor, window and all other room geometry.
- Head tracking, turning, locomotion, controller bindings and room menus.
- Existing per-zone LED colour settings; only the headboard emitter visibility is gated by viewing angle.

## Regenerate
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
