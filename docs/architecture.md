# Architecture — QuestBridge XR 0.8.1

## Native Windows OpenXR runtime (experimental)

`runtime/src/qbx_runtime.cpp` builds into `qbx_openxr_runtime.dll` on Windows x64. The runtime negotiates OpenXR API 1.0 and implements loader negotiation, a D3D11 graphics binding, a virtual stereo HMD system, session/space/action/swapchain/frame lifecycle, action-state snapshots updated by `xrSyncActions`, and a D3D11 mirror window for submitted projection layers. It intentionally advertises only `XR_KHR_D3D11_enable`.

The runtime listens for tracking/controller packets on `127.0.0.1:9945`, a feed forwarded by `QuestBridgeHost`. The host's normal network server accepts the Quest client protocol and forwards valid tracking/controller packets to this localhost feed; the runtime does not directly own the headset connection. Missing packets leave pose values at a deterministic fallback for bring-up, but the corresponding OpenXR tracking-validity flags are cleared.

The runtime manifest is shipped beside the DLL in the portable bundle. Studio can launch an OpenXR application with `XR_RUNTIME_JSON` set for that child process. The project never modifies the machine-wide active runtime registry. The mirror is currently captured by FFmpeg's Windows `gdigrab` and streamed by the existing UDP sender; this is a practical development loop, not a zero-copy compositor-to-headset pipeline.

## Quest app and room

The Android NativeActivity runs under the Quest's own OpenXR runtime to read headset/controller tracking, render the walkable cozy room, decode incoming H.264, and send QBX tracking/input datagrams to the host. In this architecture, the Quest client does not itself give PCVR games access to tracking; the host telemetry forwarder makes those packets available to the native PC runtime.

## Studio and helper

Studio edits validated JSON configuration and can launch `tools/openxr_runtime.py` behavior for Meta, active, custom, or the bundled experimental QuestBridge runtime. Environment override is process-local. The UI does not set registry keys, install kernel drivers, or claim app support beyond what the runtime implements.

## Verification boundary

The Python code/tests can be executed in a Linux/container environment, but `runtime/` is Win32/D3D11-only and must be compiled with MSVC on Windows. It has not been run by the Khronos loader or conformance suite, nor connected to actual Quest hardware here. CI success would prove compilation/package integrity only; it would not prove native game compatibility or acceptable wireless latency.
