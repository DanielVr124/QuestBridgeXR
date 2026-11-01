# QuestBridge XR — original bed with a corrected padded headboard

This update keeps the original bed body and the existing pillow arrangement, and corrects the headboard into one continuous upholstered cushion.

## Headboard
- One wide charcoal padded panel, not five separate vertical cushions.
- Broad rectangular silhouette with gently eased corners; the padding stays full without becoming an oversized oval.
- A slim, low-profile upholstered perimeter lip protects the recessed RGB channel.
- The LED emitter is rendered by the room compositor and appears only when looking almost straight at the headboard; it stays hidden from a reclined view.
- No vertical ribs, tall cushion channels, bulky double rails, exposed LED tape, diamond buttons or decorative crystal parts.

## What stays exactly as it was
- Original bed platform, rails, legs, foot end, mattress, fitted sheet, duvet and throw.
- Existing refreshed pillows and their arrangement.
- Bed position, scale, renderer anchor and collision footprint.
- Bedside cabinets, lights, TV, desk, monitor, window and all other room geometry.
- Head tracking, turning, locomotion, controller bindings and room menus.
- Existing per-zone LED colour settings.

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
