# Known functional limits — QuestBridge XR 0.8.1

## Windows OpenXR runtime

- A native Windows D3D11 OpenXR runtime/compositor now exists in source and is included in the portable build recipe. It has **not been compiled or exercised by a Windows OpenXR loader in this environment** and has not passed the Khronos Conformance Test Suite. Treat it as experimental bring-up code.
- D3D11 only. Vulkan, D3D12, OpenGL bindings and optional graphics/runtime extensions are not implemented. Apps that require them or unsupported optional extensions may fail to start.
- The compositor mirrors only projection layers to a Win32 window. No quad/cylinder/depth layers, timewarp, spacewarp, reprojection, foveation, or runtime-managed display timing are implemented.
- Video is streamed by capturing the mirror window with FFmpeg `gdigrab` and encoding H.264, so it incurs additional capture/encoding latency and is not a direct GPU swapchain transport.
- Runtime input mapping is basic; action bindings support common Touch-controller path strings. Haptic actions can be created but rumble calls are no-ops, stage space currently aliases local space, no guardian bounds are supplied, and linear/angular velocity is not reported as valid.
- The host telemetry feed must be running on localhost port 9945. If packets do not arrive, the runtime uses a stationary fallback pose, which is only for lifecycle smoke tests.
- Do not register this runtime machine-wide. Use Studio's per-process runtime launcher, and keep your known-good runtime selected globally.

## Quest and desktop path

- The Quest Android application, Windows host, and native runtime have not been compiled together or validated end-to-end on a physical Quest in this environment.
- Fake VR uses standard Windows mouse movement for apps that accept mouse-look. It cannot force Roblox DOORS to use stereoscopic rendering or guarantee its camera/input behavior.
- Virtual monitors currently mirror the same incoming source; no independent desktop/window sources, audio streaming, or robust retransmission/adaptive bitrate is implemented.
- The virtual room uses still scenery imagery rather than fully navigable photogrammetric/panoramic spaces.

## Safe defaults

- No system-wide OpenXR registry setting is changed by build or launch tools.
- The QuestBridge runtime should only be selected per process for controlled tests until it passes Windows loader/API checks and Khronos conformance tests.
- The fake-VR head-look bridge starts disabled and only sends standard mouse input to the allowlisted foreground process while enabled.
- Use UDP on a trusted private network only.
