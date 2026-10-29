# QuestBridge Studio — customization reference

QuestBridge Studio is a Python/Tkinter desktop control panel. Launch `QuestBridgeStudio.bat` on Windows or run `py -3 tools/questbridge_studio.py`. Hover every control for a longer tooltip describing its behavior, supported range, performance trade-offs, and limits. The tabs are scrollable.

## Profiles

- **Balanced** — default working values.
- **Low Latency** — reduced resolution, quick keyframe recovery, and a short incomplete-frame timeout.
- **Ultra Low Latency** — 720p/90, CBR, short VBV buffer, and short keyframe interval. This may demand significant Wi-Fi capacity.
- **High Quality** — higher resolution/bitrate and a slower encoder preset.
- **Cinema Quality** — high resolution and VBR peak headroom, with aspect-preserving image fit. Needs a capable encoder and network.
- **WiFi Stable** — moderate resolution/bitrate, less aggressive VBR peaks, and a larger receive queue. A larger queue can also increase latency under congestion.
- **Comfort Accessibility** — adjustable input deadzones/trigger curve and a modest display correction preset.
- **Network Debug** — low-rate test profile to help isolate connection problems.

Profiles are editable presets, not hardware-specific guarantees. Use **Save Profile As…** to create your own. JSON config import/export supports advanced workflows; validation runs before saving. Width, height, and FPS allow custom numeric values as well as common presets. **Restore Defaults** writes the built-in Balanced settings back to the active config without deleting named profiles.

## Settings overview

### Connection

- **PC / Quest IPv4**: identify each device on the same private LAN.
- **Host / Quest UDP port**: must match across the Windows host, Quest INI and sender.
- **Local telemetry port**: localhost-only mirror for the tracking monitor; `0` disables it.

### Video / encoding

- **Source and layout**: generated test pattern or Windows desktop capture; mono or side-by-side image layout. Desktop capture is always a 2D preview, not true stereo game capture.
- **Resolution / FPS / bitrate**: quality and workload controls. Higher resolution/FPS/bitrate increase encoder, network and decoder demand.
- **Preset and tune**: libx264 speed/quality and content/latency decisions.
- **H.264 profile**: baseline/main/high compatibility choice.
- **CBR/VBR, VBR peak factor and VBV buffer**: trade predictable throughput against quality bursts and encoder buffering.
- **Encoder threads**: `0` delegates the choice to FFmpeg; fixed values are useful for experimentation.
- **UDP send buffer**: OS-level sender queue in KiB; OS limits can clamp the request.
- **Keyframe interval**: shorter intervals help recover from lost packets but consume more bandwidth.
- **Config repeat interval**: periodic reminder of stream dimensions/layout/codec for a client that starts late or after a restart.

### Tracking and input

- **Tracking/controller toggles and update rate**: control outgoing diagnostic telemetry, not the Quest's own headset tracking/display loop.
- **Position scale and XYZ offsets**: transform outgoing reported positions for testing/calibration.
- **Thumbstick deadzone and X/Y inversion**: reduce center drift and correct axis signs.
- **Trigger deadzone/curve**: filter small trigger noise and change the analog response curve.
- **Swap hands / primary-secondary mapping**: alter packet identity or the two face-button states.
- **Grip/aim pose and LOCAL/STAGE origin**: choose controller pose convention and reference space. STAGE falls back to LOCAL if unsupported. The room floor default (-1.60 m) assumes a head-height-like LOCAL origin; when STAGE is available, try floor offset 0.0 m first because STAGE is usually floor referenced.

### Display / transport

- **Brightness, contrast, saturation, gamma**: post-decode shader corrections on the streamed image only.
- **Aspect behavior**: stretch distorts to fill, fit preserves the whole image with black bars, crop fills while trimming edges.
- **Horizontal/vertical flip and eye swap**: correct image orientation or left/right assignment.
- **Maximum frame size / incomplete-frame timeout**: bound receiver memory and how long it waits for missing UDP chunks.
- **Quest receive buffer**: request a kernel-level UDP queue size; large queues can reduce packet drops but may add latency.

## Diagnostics and connection testing

Use **Run Setup Diagnostics** to validate the active JSON, saved profiles, project files, protocol structure sizes, FFmpeg/libx264 availability, and development prerequisites. **Test PC Host Connection** sends a QBX HELLO to the configured Windows host and verifies its HELLO_ACK reply. A timeout usually means the host is not running, the PC IP/port is wrong, or local firewall/network rules block UDP. The report can be copied for troubleshooting.

## Which components read which settings?

- `tools/stream_test_video.py` reads encoder, bitrate-control, GOP, source, layout, sender socket buffer, output size and frame rate.
- `quest-client/app/src/main/cpp/main.cpp` reads Quest network, tracking/input shaping, position offsets, controller identity, and receive-buffer settings from the generated INI.
- `quest-client/app/src/main/cpp/video_decoder.cpp` applies decoded-video color, aspect, flip, and eye assignment controls in its OpenGL ES shader.
- `host/src/main.cpp` reads port arguments from Studio and emits the localhost telemetry feed.

Settings are saved to JSON; client settings are copied into `config.ini`. Restart the Quest app after installing a new INI. Restart the host/sender after changing settings consumed by those processes.

## Boundaries

These controls improve the existing prototype's sender/client/telemetry behavior. They are **not** options for a finished Windows OpenXR compositor, and they do not imply that an independent Windows runtime, PC-game stereo capture, audio transport, haptics, adaptive bitrate, or loss recovery are implemented. No option can enable a capability that is absent from the source.


## Virtual Room

- **Window scenery**: choose blossom/snow, night city, golden city, or one of two procedural scenes: an aurora sky with green/cyan curtains and stars, or a cyberpunk skyline with neon windows. These are still window views, not 360° environments.
- **Room size**: changes room bounds and visual dimensions (small/medium/large).
- **PC stream appears on**: routes the one current video source to desk monitor meshes, the TV, or both.
- **Desk monitor count**: draws one to three monitor meshes; all mirror the same incoming video feed today.
- **TV/couch/chair visibility**: show or hide the wall TV and seating furniture.
- **Joystick locomotion and turning**: left stick moves relative to the head; right stick uses snap turn (more comfortable for many users) or smooth turn. Simple collision volumes limit walking through furniture.
- **Comfort vignette**: dims the view edges during motion to help some users with comfort; it is a preference, not a guarantee against motion sickness.
- **Seat controls**: walk near the desk chair or couch, then press left-controller X to enter seated camera height. Press X again to stand or push the movement stick to stand and walk away. This is a simple virtual camera adjustment, not full-body seating physics.

Controller shortcuts while in the room: left stick click cycles monitor/TV/both; right stick click cycles scenery; left X sits/stands when near a seat. The controller shortcuts can be disabled in Studio.

## Play Modes: Fake VR vs. real PCVR

**Cozy Room / Fake VR** runs the QuestBridge room app on the headset. The PC desktop is a single 2D stream shown in that room, mirrored to both eyes. When enabled, the Windows head-look bridge maps tracked yaw/pitch to ordinary mouse movement in a foreground process listed in `config/fake_vr.ini`. It is opt-in, allowlisted, and does not modify/inject into game code. It may help with a game that accepts normal mouse-look input, but cannot provide native stereo/6DoF gameplay.

**Real PCVR** uses Meta Quest Link and Meta's PC OpenXR runtime as a separate route. The Play Modes tab can launch the PC app if installed and links to setup information. This avoids requiring SteamVR for compatible Link/OpenXR apps, while QuestBridge's own D3D11 runtime is now available as an experimental, limited-API option. It has not passed conformance or been validated on hardware and should only be launched per process. See [play modes](play-modes.md).


## Quick Network Setup

The **Quick Network Setup…** button in QuestBridge Studio detects the Windows IPv4 address selected by the PC's default route. If exactly one authorized ADB device is connected, it also tries to read that Quest's Wi-Fi IPv4 address. It only reads addresses until **Apply Found Addresses** is clicked; it does not install the APK, change the firewall, or modify the active OpenXR runtime. ADB discovery requires Android platform-tools and an authorized USB/Wi-Fi debugging connection. When no headset is connected, apply the PC address and fill in the Quest address later in the Connection tab.
