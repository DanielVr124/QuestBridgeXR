# Play modes — QuestBridge XR 0.8.1

## Cozy room / fake VR

The Quest standalone app renders a locally simulated room and displays a streamed PC video source on virtual monitors and a TV. Its optional fake-VR head-look bridge maps headset yaw/pitch to normal mouse deltas in a focused, allowlisted Windows app. This is not a stereo VR conversion; Roblox DOORS and other games must accept mouse-look, and room/game camera semantics vary by application.

## Real PCVR: experimental QuestBridge runtime

The portable Windows bundle is intended to include `runtime/qbx_openxr_runtime.dll` and `runtime/questbridge_openxr_runtime.json`. Studio can launch one selected OpenXR EXE with `XR_RUNTIME_JSON` pointing at this manifest for that child process only. The runtime implements a limited D3D11 API surface and mirrors submitted stereo projection views into a window titled `QuestBridge XR Compositor`.

For an initial test, use a simple D3D11 OpenXR sample before attempting a game engine. Start the Windows host and Quest client to provide the tracking/controller feed, select the runtime from Studio, and then run:

```powershell
py -3 .\tools\stream_test_video.py --source mirror --layout sbs --width 1280 --height 720 --fps 72 --bitrate 12M
```

The mirror uses FFmpeg `gdigrab` capture and H.264 encode; it is a development path, not a low-latency, zero-copy VR compositor. There is no asynchronous timewarp/reprojection. The runtime has not been built/tested on actual Windows/Quest hardware from this environment and has not passed Khronos conformance. It can fail with apps that require unsupported extensions. Do not register it as the machine-wide active runtime.

## Fallback: Meta Quest Link

If an app does not work with the experimental runtime, use Meta Quest Link and its PC OpenXR runtime for compatible applications. This avoids the SteamVR dependency for apps supported by Meta's runtime, but it is not a QuestBridge-owned runtime. Studio's per-process launcher doesn't change the active-runtime registry.
