# QuestBridge XR 0.8.1 — experimental Windows OpenXR runtime bring-up

QuestBridge XR is an experimental Windows-to-Meta-Quest streaming project with a native Quest OpenXR client, a UDP video/tracking protocol, a Windows diagnostic/input-bridge host, an FFmpeg sender, telemetry tools, and a highly customizable desktop control panel. This source revision adds a portable Windows build path and CI workflow that can produce a Windows Studio/host package and a Quest debug APK, in addition to a locally rendered walkable 3D room with 88 selectable scenery environments (including blossom, neon cities, forests, oceans, mountains, space, fantasy, seasonal, and synthwave scenes) shown through its environment window, a desk with mirrored monitors, a TV, a couch, a chair, joystick locomotion, snap/smooth turning, and proximity-based virtual seating.

**Important status:** this revision adds an **experimental native Windows OpenXR runtime DLL and D3D11 mirror compositor** under `runtime/`. Unlike earlier versions, this is no longer just a manifest selector: it has loader negotiation, a defined runtime function table, a D3D11 session/swapchain implementation, stereo pose/view reporting, controller action-state mapping, and a mirror window that draws submitted projection eye images side by side. The runtime is deliberately not registered globally and is not yet Khronos-conformance-tested or validated on physical Quest hardware. Expect limited app compatibility and bugs; test only through Studio's per-process runtime override. If an app fails, switch back to Meta Quest Link/your existing known-good runtime. No prebuilt DLL, EXE, or APK is included in the source ZIP; use the Windows/Android build workflow to produce them. See [runtime notes](runtime/README.md), [known limitations](docs/known_limits.md), and [build instructions](docs/build-artifacts.md).
## Launch QuestBridge Studio

On Windows, double-click `QuestBridgeStudio.bat`, or run:

```powershell
py -3 .\tools\questbridge_studio.py
```

Studio exposes **75 settings** across Connection, Video, Tracking & Input, Display, Advanced, Virtual Room, and Fake VR. The Play Modes tab explains the difference between cozy-room/fake head-look mode and real PCVR through Meta Quest Link. Hover a setting to see a detailed explanation of what it does, how it changes behavior, and its trade-offs. Tabs scroll when needed. All values are range-validated before saving. Width, height, and FPS fields accept custom values as well as presets; **Restore Defaults** resets the active config without deleting named profiles.

Starter profiles include Balanced, Low Latency, Ultra Low Latency, High Quality, Cinema Quality, WiFi Stable, Comfort Accessibility, and Network Debug. These are starting points, not universal best settings; network quality and PC hardware vary. You can save/import/export JSON profiles and export or install the Quest-side `config.ini` through ADB.

The active settings file is `config/questbridge.json`. Studio merges new defaults into older schema-v1 settings files, so earlier profiles keep working when new fields are added.

## Implemented source components

- **Windows UDP diagnostic host (`host/`)**: Winsock connection handshake, basic tracking/controller packet validation, video ingress counters, and a localhost telemetry forwarder.
- **Native Quest client (`quest-client/`)**: Android NativeActivity using the Khronos OpenXR Android loader; HMD pose sampling; Touch pose/actions; configurable tracking/input shaping; UDP receive/reassembly; MediaCodec decode; OpenGL ES stereo presentation; a simple world-anchored 3D room, scenery-window assets, virtual monitors/TV, joystick movement, turning and near-seat chair/couch interactions.
- **FFmpeg sender (`tools/stream_test_video.py`)**: generated H.264 test pattern, Windows desktop capture, or the experimental runtime's titled side-by-side mirror window (`--source mirror`); UDP packetization under the project's 1200-byte datagram cap; H.264 rate and encoder controls. `mirror` transfers the compositor's eye images through a capture/encode stage, not zero-copy GPU transport or timewarp.
- **QuestBridge Studio (`tools/questbridge_studio.py`)**: 75 settings with detailed hover tooltips; scrolling tabs; profile selection; JSON import/export; Quest INI export and optional ADB install; launch controls for the host, sender and telemetry monitor; setup diagnostics and an optional real UDP HELLO/HELLO_ACK host connection test; Play Modes help with Meta Quest Link launch and the Roblox DOORS page. The diagnostics report is copyable and does not modify OpenXR registry settings.
- **Telemetry tools**: `tools/simulate_quest.py` emits synthetic packets; `tools/monitor_telemetry.py` decodes the host's localhost tracking feed.
- **Native experimental OpenXR runtime (`runtime/`)**: Windows-only D3D11 runtime DLL, loader negotiation entry point, instance/session/space/action/swapchain/frame API subset, and a mirror compositor window. It receives tracking/controller telemetry forwarded by the Windows host. It is not yet conformance-tested, and only its supported D3D11/API surface can work.
- **OpenXR Runtime Helper**: `tools/openxr_runtime.py` inspects runtime manifests and launches a chosen app with a per-process `XR_RUNTIME_JSON` override. It can select the experimental QuestBridge runtime from the portable bundle; it never changes the system-wide active-runtime registry.
- **Tests**: standard-library Python tests cover config validation/profiles/INI export, packet sizes, chunking, telemetry decoding, FFmpeg command construction, CBR/VBR options and profile validation.

## Ready-to-run Windows packaging

The CI workflow is configured to publish two user-facing artifacts after it runs successfully: a portable ZIP and a per-user Windows Setup EXE. The installer creates shortcuts, includes the Quest debug APK, seeds the default settings, and preserves the active configuration/named profiles across upgrades and uninstall. Neither artifact is built in this environment; a GitHub Actions run or Windows build machine is needed to create the binary packages. FFmpeg is not bundled, and the Quest APK still requires Developer Mode/ADB to install.

## Build a portable Windows package

On Windows 10/11, install Visual Studio 2022 Build Tools with **Desktop development with C++**, CMake 3.24+, Git (for the pinned Khronos OpenXR-SDK dependency), and Python 3.11+. Double-click `Build-QuestBridgeXR.bat`, or use a VS 2022 Developer PowerShell:

```powershell
.\scripts\build-all.ps1
```

This builds the native UDP host and experimental D3D11 OpenXR runtime DLL, packages Studio/helper tools with PyInstaller, and creates `dist\QuestBridgeXR-0.8.1-Portable.zip` plus a SHA-256 checksum. If Gradle and the Android SDK/NDK are available, the Quest debug APK is built and included; without them, the Windows package records the omission in `BUILD-INFO.txt`. FFmpeg with `libx264` is required for mirror streaming and is not bundled.

A GitHub Actions workflow at `.github/workflows/build-release.yml` can build the Windows portable package and Android Quest debug APK on hosted runners. See [Build Artifacts](docs/build-artifacts.md) for how to run it. The Play Modes tab also includes a read-only runtime inspector and a per-process runtime launcher; see [OpenXR Runtime Helper](docs/openxr-runtime-helper.md). The APK is a debug build and must be installed by ADB or copied through Android Studio.

## Build the Windows host (developer-only)

Requirements: Windows 10/11, Visual Studio 2022 Build Tools with **Desktop development with C++**, and CMake 3.24+.

In a Developer PowerShell for VS 2022, from the repository root:

```powershell
.\scripts\build-windows.ps1
.\build\host\Release\QuestBridgeHost.exe
```

The host listens on UDP `network.host_port` (default 9944) and forwards client telemetry to `127.0.0.1:network.telemetry_port` (default 9945). Allow inbound traffic only on a trusted private network. Set telemetry port to `0` to disable it.

Test the host without a headset with a second terminal:

```powershell
py -3 .\tools\simulate_quest.py --host 127.0.0.1 --port 9944
py -3 .\tools\monitor_telemetry.py --port 9945
```

## Build and install the Quest client

Requirements: Android Studio, JDK 17, Android SDK Platform 35, NDK `27.2.12479018`, CMake `3.22.1`, and a Meta Quest with Developer Mode and USB debugging enabled.

1. Open `quest-client/` in Android Studio and let Gradle sync.
2. Build the debug APK (`:app:assembleDebug`).
3. In the combined portable ZIP, you can instead run `Install-QuestClient.bat` or click **Install & Launch Quest APK** in Studio; these require ADB, USB debugging, and exactly one authorized Quest. For a local source build, install `quest-client/app/build/outputs/apk/debug/app-debug.apk` with ADB.
4. Launch QuestBridge XR from the Quest's Unknown Sources library section once.
5. Set the PC and Quest LAN IPv4 addresses in Studio and choose the desired options.
6. Click **Install Quest INI via ADB**, or export `config.ini` and push it to `/sdcard/Android/data/com.questbridge.xr/files/config.ini` manually. Restart the app after changing client settings.
7. Inspect `adb logcat -s QuestBridgeXR` and the Windows host console for bring-up diagnostics.

This Android source has not been built and hardware-validated in this development environment. Treat device compatibility and video latency as unverified until tested on your headset.

## Test video transport

Install FFmpeg with `libx264`; Windows desktop mode also needs `gdigrab`. Start the Windows host and Quest app, then run:

```powershell
py -3 .\tools\stream_test_video.py --config .\config\questbridge.json
```

For a low-bandwidth mono test:

```powershell
py -3 .\tools\stream_test_video.py --source test --layout mono --width 1280 --height 720 --fps 60 --bitrate 5M
```

For a 2D desktop preview:

```powershell
py -3 .\tools\stream_test_video.py --source desktop --layout mono --width 1280 --height 720 --fps 60 --bitrate 5M
```

For the experimental runtime's stereo mirror, after a compatible OpenXR app is running and `QuestBridge XR Compositor` is visible:

```powershell
py -3 .\tools\stream_test_video.py --source mirror --layout sbs --width 1280 --height 720 --fps 72 --bitrate 12M
```

This uses Windows `gdigrab` to capture the mirror window then H.264-encodes it; it adds latency and is not a direct GPU/VR compositor transport. In the portable package, choose **Play Modes → Launch an EXE with QuestBridge runtime (experimental)** to run a test application with the runtime only for that process. Do not set it as the system-wide active runtime until it has passed testing on Windows hardware and Khronos conformance checks.

The sender supports H.264 Annex-B output only. Add `--duration-seconds 5` for a finite five-second stream test; omitting it keeps streaming until stopped. `--rate-control cbr` aims for a tighter target bitrate; `vbr` allows bursts using `--maxrate-factor`. The `--vbv-buffer-ms`, `--threads`, `--tune`, `--h264-profile`, and `--udp-send-buffer-kib` settings can also be set through Studio.

## Setup diagnostics

Use the **Run Setup Diagnostics** button in Studio, or run:

```powershell
py -3 .\tools\doctor.py --config .\config\questbridge.json
```

To send a real UDP QBX HELLO packet to the configured Windows host and wait for HELLO_ACK:

```powershell
py -3 .\tools\doctor.py --config .\config\questbridge.json --probe-host
```

The handshake test requires the Windows host to be running and the configured PC IP/UDP port to be reachable. The diagnostics tool is read-only and does not set or change the active Windows OpenXR runtime.

## Run tests

```powershell
py -3 -m unittest discover -s tools -p "test_*.py" -v
```

The latest local test run executes **75 tests headlessly** (the Tk UI test class is skipped without a display) and **77 tests pass under Xvfb**, including the Studio UI tests. Run `xvfb-run -a python3 -m unittest discover -s tools -p 'test_*.py' -v` on a machine with Xvfb. A fresh local FFmpeg-to-UDP smoke test delivered **121 valid datagrams** (3 stream-config packets and 118 frame chunks); all headers and payload lengths were valid, there were no malformed packets, and the largest datagram was 1,200 bytes. It uses a generated test pattern, not a game capture. These tests verify Python behavior and local transport, not Windows C++ compilation or actual Quest graphics/decoder behavior. This environment cannot validate the Windows MSVC runtime build or physical-headset rendering.

## Current functional boundary

```text
Implemented in source (not yet device-validated):
  Quest OpenXR tracking/input + local 3D cozy-room renderer
  FFmpeg H.264 test/desktop sender ------> Quest UDP/MediaCodec/GLES display path
  Optional allowlisted Windows mouse-look bridge for flat-screen games
  Studio settings/profiles/tooltips + Quest INI export/ADB install
  Real PCVR launch guidance through the separate Meta Quest Link runtime

Still incomplete or not yet validated:
  The native OpenXR runtime DLL has not yet been compiled by MSVC or loaded on Windows
  OpenXR 1.0 coverage is limited; common extensions and alternative graphics APIs are missing
  No reprojection/timewarp and no direct GPU-swapchain-to-Quest transport
  The mirror path is window capture + FFmpeg encode, not a production compositor pipeline
  Independent capture streams for separately managed virtual monitors are not implemented
  End-to-end validation on Windows + a physical Quest has not occurred
```

QuestBridge XR does not change the Windows registry or register itself as the active OpenXR runtime. A portable build is not a runtime replacement; the “Real PCVR” path uses the existing Meta Quest Link OpenXR runtime for compatible apps. Keep your existing Meta/SteamVR configuration intact while this is prototype software.
