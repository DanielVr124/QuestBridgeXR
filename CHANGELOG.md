# Changelog

## 0.8.1 — OpenXR runtime correctness hardening (experimental)

- Corrected duplicate validation branches in action-set/action creation and interaction-profile binding setup.
- Added action name length/duplicate checks, strict one-time action-set attachment, and core haptic action creation (haptic output remains a documented no-op).
- Gated the D3D11 requirements proc lookup on the extension being enabled.
- Reject unsupported API-layer requests rather than silently ignoring layers the runtime does not implement.
- Kept runtime API negotiation truthful at OpenXR 1.0.0; the current command surface is intentionally limited.
- Revalidated the source-contract and portable build workflow. This version still requires a successful native Windows build, loader smoke test, and physical Quest test before it can be described as working or suitable as a default runtime.

## 0.8.0 — first native D3D11 OpenXR runtime bring-up (experimental)

- Added `runtime/qbx_openxr_runtime.dll` source with OpenXR loader/runtime negotiation, system/session lifecycle, D3D11 swapchains, basic spaces/actions, frame submission, and a D3D11 side-by-side mirror window.
- Added `xrEnumerateInstanceVersion` and honest OpenXR API 1.0 negotiation; implemented per-hand action-state caching/change detection during `xrSyncActions`, validated frame sequencing, and stricter tracking-validity flags. Added controller/headset ingestion from the host localhost telemetry feed and mirror-frame capture mode (`--source mirror`) in the H.264 sender.
- Added the pinned Khronos OpenXR-SDK dependency in runtime CMake, a Windows portable-build step for the DLL/manifest, CI deliverable checks, and an opt-in per-process runtime launch button in Studio.
- **Validation boundary:** Python source tests are run here, but the native runtime cannot be compiled in this Linux environment because the Windows SDK/MSVC toolchain is unavailable. It has not run on the Khronos loader, passed conformance tests, or been validated on a Quest. Do not set it as the machine-wide active runtime. It is a first bring-up implementation, not yet a reliable SteamVR replacement.


## 0.7.0 — per-process OpenXR runtime helper and launch UX

- Added a read-only OpenXR runtime discovery utility for current environment overrides, Windows registry manifest paths, and common Meta/SteamVR installs.
- Added a per-process launcher that sets `XR_RUNTIME_JSON` only in a child application's environment; it never edits Windows' machine-wide active-runtime registry.
- Added Studio actions to inspect discovered runtimes or launch a selected EXE using Meta Quest Link or the currently active runtime.
- Included the helper in the standalone Windows package and CI artifact checks.
- Added seven unit tests for manifest validation, runtime selection, process-only environment overrides, and safe launch failure behavior.
- The custom QuestBridge Windows OpenXR runtime/compositor remains unimplemented; Meta Quest Link is still the practical no-SteamVR route for compatible native PCVR apps. The Android APK and Windows executables are not built in this environment.

## 0.6.1 — release-ready packaging path

- Added a tagged GitHub Release job which publishes the combined Windows portable package, Quest APK, and SHA-256 checksum after CI succeeds.
- Added workflow checks that verify the Windows executable payloads and the Android APK contents before upload.
- Added a local combined build script that produces a portable ZIP and checksum, including the Quest APK when Android build tools are installed.
- Added explicit Meta Quest immersive NativeActivity metadata to the Android manifest.
- Bumped the Quest Android app version to 0.6.1.
- This does not add the missing standalone Windows OpenXR runtime/compositor; native PCVR still routes through Meta Quest Link.

## 0.6.0 — portable build pipeline

- Added a Windows portable-package builder that builds the C++ host and bundles Studio plus diagnostic/streaming helper executables with PyInstaller.
- Added shared path resolution so the same tools work from source checkouts and a portable release folder.
- Added GitHub Actions jobs to build the Windows package, build the Quest debug APK, and combine them into a downloadable ZIP.
- Added automated build-artifact and OpenXR compatibility notes; no incomplete runtime is registered.
- Added path-resolution tests and updated the Quest Android version to 0.6.0.


## 0.5.0 — cozy room, chair/couch seating, and play-mode guidance (source, not device-validated)

- Added a GLES room renderer with small/medium/large room footprints, 3 user-provided scenery-window textures, desk monitors, a wall TV, couch, desk chair, and color palettes per environment.
- Added controller-driven room movement/turning, monitor → TV → both routing, scenery cycling, proximity-based virtual seating, and stand-up controls. The room remains a 3D local scene; window scenery is a still image rather than a full panorama.
- Added a Studio Play Modes tab separating Cozy Room/Fake VR from real PCVR through the installed Meta Quest Link runtime, with buttons to launch Meta Quest Link and open Roblox DOORS.
- Connected the experimental head-look bridge to accepted Quest TRACKING packets and kept it opt-in, allowlist-gated, foreground-window-gated, and based on normal Windows mouse input only.
- Updated the Quest Android package version to 0.5.0 and expanded setup/known-limitations documentation.
- Validation in this container: 45 tests pass under Xvfb; Python/Tk UI source checked under Xvfb. A one-second local FFmpeg-to-UDP smoke test delivered 103 valid packets (3 config packets + 100 frame chunks) under the 1,200-byte cap. The Android app and Windows host are not compiled here, and physical-Quest behavior remains unverified. The standalone QuestBridge Windows OpenXR runtime/compositor is still not implemented.


## 0.4.2 — stream tuning, diagnostics, and Studio hardening (source, not device-validated)

- Expanded Studio and configuration support for H.264 tune/profile and CBR/VBR rate control, VBV sizing, thread count, UDP socket buffers, trigger shaping, tracking offsets, controller hand swapping, saturation/gamma, aspect handling, and image flips.
- Added detailed hover explanations for all 49 controls, eight tuning profiles, custom resolution/height/FPS values, and Restore Defaults that preserves named profiles.
- Added setup diagnostics, an optional live UDP host handshake probe, and a copyable report window in Studio.
- Added a finite-duration video-sender mode for reproducible transport tests.
- Historical validation: 34 Python tests passed; Studio widget/custom-mode smoke passed under Xvfb; local FFmpeg-to-UDP smoke produced 224 correctly formed datagrams with none over the 1,200-byte MTU cap. Windows and Android builds and physical Quest runtime behavior remain unvalidated.

## 0.4.1 — diagnostics and customization hardening (source, not device-validated)

- Added a setup report and a real UDP HELLO/HELLO_ACK test from Studio, with copyable diagnostic output.
- Added unit tests for the host handshake packet and verified full Studio tooltip coverage.
- Added arbitrary custom width/height/FPS entry (within validated bounds), a Restore Defaults control that preserves saved named profiles, and an optional finite-duration FFmpeg test stream.
- Python test suite passes with 34 tests in this environment; Studio widget construction and custom resolution/FPS editing passed under a virtual display.

## 0.4.0 — extensive configuration and video tuning (source, not device-validated)

- Added detailed hover tooltips for every Studio setting, plus scrollable tabs so larger configuration pages remain accessible.
- Expanded Studio to 49 controls for H.264 profile/tune, CBR/VBR rate control, maxrate factor, VBV buffer duration, encoder threads, UDP socket buffers, trigger curve/deadzone, XYZ telemetry offsets, hand swapping, saturation, gamma, aspect fit/crop/stretch, and image flips.
- Wired the new encoding controls into FFmpeg command construction and UDP send-socket buffer sizing.
- Wired Quest-side trigger shaping, position offsets, hand identity swapping, receive-buffer requests, and shader-side saturation/gamma/aspect/flip settings into the native client source and Quest INI.
- Added Ultra Low Latency, WiFi Stable, Cinema Quality, and Comfort Accessibility starter profiles.
- Added range/profile/INI tests and CBR/VBR FFmpeg option tests.
- Android NDK build, Windows MSVC build, physical Quest video decode, and runtime end-to-end validation were not available here.
- The standalone Windows OpenXR runtime/compositor and arbitrary PCVR stereo swapchain capture remain unimplemented; this release does not register a Windows runtime or modify the registry.

## 0.3.1 — telemetry bridge and streaming reliability (source, not device-validated)

- Added a configurable Windows localhost UDP feed for accepted HMD tracking and controller packets.
- Added `tools/monitor_telemetry.py` to inspect tracking/controller telemetry.
- Fixed sender monotonic timestamps and aligned frame limits with the chunk cap.

## 0.3.0 — QuestBridge Studio and client tuning (source, not device-validated)

- Added the Tkinter control panel, validated JSON profiles, Quest INI export and optional ADB installation.
- Added source for a native Quest OpenXR client and FFmpeg H.264 test sender.

## 0.2.1 — desktop preview sender

- Added optional Windows FFmpeg `gdigrab` desktop preview. The same 2D image is shown in both eyes; it is not VR capture.

## 0.2.0 — native Quest transport bring-up

- Added Android NativeActivity/OpenXR client source, HMD/Touch input telemetry, UDP frame reassembly, MediaCodec decoding, OpenGL ES presentation, and protocol tests.
