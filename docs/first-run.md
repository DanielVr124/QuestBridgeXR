# First run — QuestBridge XR 0.8.1

## Windows build

1. Install Visual Studio 2022 Desktop development with C++, CMake 3.24+, Git, and Python 3.11+.
2. Run `Build-QuestBridgeXR.bat` from a Windows terminal. The first runtime build fetches the pinned Khronos OpenXR SDK dependency.
3. Start `QuestBridgeStudio.exe` and review **Diagnostics**. FFmpeg with `libx264` is not bundled and is required for streaming.

## Quest headset

1. Enable Developer Mode and USB debugging, connect the headset, and accept the authorization prompt.
2. Use Studio's **Install & Launch Quest APK** if the combined package includes `QuestClient/QuestBridgeXR-Quest-debug.apk`.
3. Set the PC/Quest LAN IP configuration, install the exported Quest INI, and start the Quest app.
4. Start `QuestBridgeHost.exe`; allow UDP on the trusted private network if Windows Firewall asks.

## Experimental Windows OpenXR runtime

The portable package includes `runtime/qbx_openxr_runtime.dll` and `runtime/questbridge_openxr_runtime.json`. Keep your global OpenXR runtime unchanged.

1. Start the host and Quest client so the runtime can receive tracking/controller packets via the host's localhost telemetry feed on UDP port 9945.
2. In Studio's **Play Modes** tab choose **Launch an EXE with QuestBridge runtime (experimental)**, then select a simple D3D11 OpenXR app such as a locally built sample. This applies a process-local `XR_RUNTIME_JSON` override only.
3. When projection frames are submitted, a window titled `QuestBridge XR Compositor` mirrors the app's left/right views side by side.
4. To send that mirror to Quest, run `py -3 .\tools\stream_test_video.py --source mirror --layout sbs --width 1280 --height 720 --fps 72 --bitrate 12M` in a terminal with FFmpeg.

This is **not yet a production SteamVR replacement**: the runtime has not passed an MSVC build or Khronos conformance test in this environment, has a limited D3D11-only API surface, and its mirror stream adds latency. If an app crashes or fails to start, stop it and use Meta Quest Link or your existing known-good runtime. Do not activate QuestBridge system-wide.
