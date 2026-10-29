# QuestBridge XR 0.8.1 validation report

## Changes in this revision

- Added a Windows-only D3D11 OpenXR runtime DLL source with the OpenXR loader negotiation export and an implemented subset of runtime entry points for instances, systems, sessions, spaces, basic actions, swapchains, frame lifecycle, and stereo view locations.
- Added the D3D11 mirror compositor window `QuestBridge XR Compositor`, which draws submitted projection views side by side.
- Added a Windows build step that fetches the Khronos OpenXR SDK headers and builds `qbx_openxr_runtime.dll`, then packages it beside a relative-path runtime manifest.
- Added opt-in per-process runtime selection in Studio; the helper never writes the global active-runtime registry.
- Added FFmpeg mirror-window capture mode for sending the side-by-side compositor preview to Quest.
- Added static source-contract tests for runtime entry points, manifest, and packaging path.
- Hardened unsupported API-layer and extension-proc handling, action-set/action name validation, duplicate-name errors, repeated action-set attach detection, and core haptic action creation (haptic output remains a documented no-op).

## Tests run in this environment

- Python unit tests cover configuration, profiles, host handshake, packet format/limits, sender command construction, runtime manifest selection, runtime source/package contracts, room assets and hover-help coverage.
- Headless suite result: **75 tests executed with 1 UI test class skipped** because a display was not available. Full Xvfb suite: **77 tests passed** including the Tk settings/help UI checks.
- A fresh FFmpeg-to-UDP loopback smoke test exited with code 0 and received **121 valid datagrams**: 3 video-config packets and 118 frame chunks. All packet headers and payload lengths validated; maximum datagram size was 1,200 bytes. This used a generated test pattern, not a game or Quest.

## Explicitly not validated

This environment does not provide the Windows SDK/MSVC toolchain. Therefore the C++ runtime DLL has **not** been compiled here, loaded by the Khronos OpenXR loader, run with an OpenXR application, or checked against the Khronos OpenXR Conformance Test Suite. This source package is not a precompiled binary release.

The D3D11 runtime has a deliberately limited feature/extension surface. It does not implement timewarp/spacewarp, direct Quest streaming from GPU swapchains, Vulkan/D3D12 bindings, many optional OpenXR extensions, or hardware haptic output. The mirror-streaming route adds window-capture and H.264 encode latency. The real PCVR path has not been validated against Roblox DOORS or any other specific game.

Keep the existing system runtime unchanged. Build the DLL on Windows and first test a basic D3D11 OpenXR sample through Studio's per-process runtime launch. If the build or sample fails, use the failure logs to correct the runtime before trying games; do not register it system-wide.
