# Immersive Worlds, Monitors, Movement, and Atmosphere

QuestBridge XR now has the first native foundation for an explorable-world experience alongside the original Workspace room.

## World modes

- **Explore** renders a lightweight procedural 3D world with themed architectural layouts, distant scenery, large walkable bounds, and basic collision against interior maze walls.
- **Workspace** keeps the bounded desk, chair/couch, scenery window, and wall TV.
- Both modes use the existing native OpenXR/GLES renderer and decoded PC video texture.

The environments are intentionally built from reusable geometry templates and palette families. This allows hundreds of named scenes without requiring hundreds of separately authored high-poly asset packs. The visual style is stylized and procedural, not a promise of hand-authored, photorealistic worlds.

## Monitor behavior

- **World** keeps virtual screens anchored at the workstation.
- **Follow** renders them in front of the view, suitable for moving around while keeping the PC screen available.
- **Summon** uses floating screens and the right-controller B button toggles their visibility.
- The left thumbstick click still cycles Monitor / TV / Both; the right thumbstick click cycles the environment; left X toggles the seated camera height; left Y switches Explore / Workspace; right B toggles monitor visibility.

Every monitor currently shows the same decoded PC stream. Separate simultaneous desktop captures, hand/controller raycast grabbing, freeform drag-and-drop placement, and independently saved per-environment monitor transforms are not implemented yet.

## Smooth locomotion

Explore mode uses eased joystick acceleration/deceleration, diagonal-speed normalization, environment bounds, and a simple collision test for interior wall segments. Smooth turning remains optional because snap turns are more comfortable for some VR users. The comfort vignette remains optional.

These are comfort-oriented foundations, not a guarantee that every user will be free of motion discomfort. Start slowly, stop when uncomfortable, and keep physical play space clear.

## Atmosphere audio

The native client synthesizes a quiet, continuous ambience layer using low-level filtered noise and soft tones selected by environment family: dreamlike drone, interior room tone, airy wind, water-like noise, rain-like hiss, cozy crackle texture, or a restrained cosmic hum. It is generated in real time with OpenSL ES, not loaded from a set of recorded sound files. This is a first ambience bed; true positional sources, room acoustics, footsteps, surface-specific footsteps, and authored soundscape loops are future upgrades.

Atmosphere audio can be turned off in Studio. The feature should stay subtle and should never block system audio or replace safety-related Quest feedback.

## Performance goals and known limits

Geometry uses primitive boxes and shared GLES buffers to keep the prototype lightweight; the renderer uses no high-poly model packs or dynamic shadow maps. The Quest Android CI workflow compiles the implementation, but a successful build cannot establish stable frame timing or audio quality on a physical Quest. Test the APK on the target headset and lower scene density if profiling reveals frame-time spikes.
