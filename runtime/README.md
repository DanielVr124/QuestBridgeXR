# QuestBridge XR Experimental Windows OpenXR Runtime

This directory contains the project's **first native Windows OpenXR runtime bring-up**, separate from `tools/openxr_runtime.py` (the manifest selector/launcher). The runtime DLL exposes the Khronos loader/runtime negotiation entry point and a limited D3D11-backed implementation of instance/system/session, reference/action spaces, actions, D3D11 swapchains, stereo view locations, frame lifecycle, and a side-by-side mirror compositor.

**Status: experimental and unvalidated on Windows hardware.** A native implementation now exists in source, but it is not a production-ready or Khronos-conformance-certified SteamVR replacement. Do not make it the machine-wide active runtime. Use only the per-process launcher for controlled tests, and keep your known-good Meta/SteamVR configuration intact.

## Build on Windows

Requirements: Windows 10/11 x64, Visual Studio 2022 with *Desktop development with C++*, CMake 3.24+, Git, and network access for the pinned Khronos OpenXR-SDK source dependency.

```powershell
cmake -S runtime -B build/runtime -A x64
cmake --build build/runtime --config Release
```

The portable build script performs this step and places these files beside each other in `runtime\`:

- `qbx_openxr_runtime.dll`
- `questbridge_openxr_runtime.json`

The manifest uses a relative library path, avoiding an absolute machine-specific path. No registry writes or self-registration are performed.

## Test with an OpenXR application

1. Build/extract the portable package and start `host\QuestBridgeHost.exe`.
2. Install/start the Quest client and connect it to the PC on the same trusted LAN. The runtime receives tracking/controller QBX datagrams forwarded on `127.0.0.1:9945`. If they are absent, a fixed fallback pose is provided for lifecycle tests only.
3. In Studio, use **Play Modes → Launch an EXE with QuestBridge runtime (experimental)** to choose a simple D3D11 OpenXR test application. This sets `XR_RUNTIME_JSON` for the child process only.
4. When the application's first projection frame is submitted, the runtime creates a window titled **QuestBridge XR Compositor** and draws the left/right projection subimages side by side.
5. To send the mirror to the Quest client, run a second terminal with FFmpeg available:

   ```powershell
   py -3 .\tools\stream_test_video.py --source mirror --layout sbs --width 1280 --height 720 --fps 72 --bitrate 12M
   ```

`gdigrab` window capture followed by CPU-visible H.264 packetization/encoding is a bring-up transport; it is not zero-copy, does not perform timewarp/reprojection, and may add noticeable latency. Keep the mirror window visible to stream it; closing it minimizes the window. Test with a simple OpenXR sample before attempting a game engine or Roblox.

## Currently implemented surface

- Runtime loader negotiation interface v1; advertises only `XR_KHR_D3D11_enable`, gates its command lookup on the enabled extension, and rejects API layers it cannot implement.
- D3D11 session creation and runtime-owned swapchain textures, exposed via `XrSwapchainImageD3D11KHR`.
- Primary stereo view configuration, a fixed nominal 90 Hz frame loop, frame begin/end sequencing, event polling, basic reference/action spaces, and action state updates driven by `xrSyncActions`. The runtime honestly negotiates OpenXR API 1.0; it does not expose the OpenXR 1.1 `xrLocateSpaces` command.
- Basic action bindings mapped to the QBX controller packets; action/action-set names are validated, duplicate names are rejected, boolean/float/vector input state is cached per hand, and changes are tracked on `xrSyncActions`. Haptic action objects can be created, but haptic requests are no-ops because the wire protocol does not carry rumble commands.
- A D3D11 shader-based mirror window that samples released projection images and places left/right views in separate halves.
- Tracking ingestion on localhost port 9945; no device tracking is fabricated beyond a fixed fallback for development bring-up.

## Known gaps / why this is not yet a SteamVR replacement

- It has **not been compiled by MSVC from this environment**, run on a real Windows OpenXR loader, or checked by the Khronos OpenXR Conformance Test Suite. The C++ source, CMake project, Windows CI build step, and source-contract tests are present, but only the Python portions have been tested here.
- D3D11 only; no Vulkan, D3D12, or OpenGL runtime binding. Some games require optional OpenXR extensions that are not implemented and will fail instance creation or function lookup.
- Only a focused OpenXR 1.0 core API surface; API layers, OpenXR 1.1-only commands, Vulkan/D3D12/OpenGL, and most optional extensions are unsupported. Controller interaction profiles and input mapping are intentionally basic. Haptics are no-ops and controller hand pose conventions need physical validation.
- The compositor mirrors only the submitted projection layer; no quad/cylinder/depth layers, spacewarp, asynchronous timewarp, distortion correction, foveated rendering, or robust frame pacing.
- The headset stream is captured from the mirror window through FFmpeg. It is not a direct GPU surface transfer and adds latency.
- The host-to-runtime feed and Quest playback have not been tested end-to-end on hardware. Runtime API compatibility and display behavior remain unverified.

Before treating the runtime as reliable, build on Windows, run the OpenXR SDK's `hello_xr` sample and loader tests, run the Khronos Conformance Test Suite, then test an actual Quest connection under controlled conditions. Do not register it globally merely to see if it starts.
