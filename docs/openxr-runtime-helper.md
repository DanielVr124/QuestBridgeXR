# OpenXR Runtime Helper

The helper can launch a Windows OpenXR app with either an installed runtime or the experimental QuestBridge D3D11 runtime shipped in the portable bundle. The QuestBridge runtime is not conformance-tested and should only be tried with a per-process override. Meta Quest Link remains the reliable fallback for compatible apps.

## What it does

- Reads the current process' `XR_RUNTIME_JSON` override, Windows OpenXR `ActiveRuntime` registry values, and common Meta Quest Link / SteamVR manifest locations.
- Validates the manifest's basic JSON shape and `runtime.library_path` field.
- Can launch a chosen Windows executable with `XR_RUNTIME_JSON` set only in that child process' environment.
- Does **not** modify the machine-wide registry, change the active runtime, inject code, or touch the game process after launching it.

`XR_RUNTIME_JSON` is an OpenXR loader override. When it is present, the loader uses that manifest instead of discovering the machine-wide registered active runtime. This makes it useful for a per-application launch choice, as long as the selected runtime is installed and compatible with the target application.

## Studio use

Open **Play Modes**. Use **Inspect installed OpenXR runtimes** to view discovered manifests. Use **Launch an EXE with Meta runtime…** to select a Windows OpenXR app and launch it with the Meta manifest, or **Launch an EXE with active runtime…** to use the runtime the loader would normally select. The helper asks for confirmation and reports the launched process ID.

## Command line

List detected runtimes:

```powershell
py -3 .\tools\openxr_runtime.py --list
```

Launch a compatible app using Meta Quest Link without changing global settings:

```powershell
py -3 .\tools\openxr_runtime.py --runtime meta --app "C:\Path\To\OpenXRApp.exe"
```

Launch using the machine's active runtime:

```powershell
py -3 .\tools\openxr_runtime.py --runtime active --app "C:\Path\To\OpenXRApp.exe"
```

Use a custom manifest:

```powershell
py -3 .\tools\openxr_runtime.py --runtime qbx --app "C:\Path\To\OpenXRApp.exe"

# Or use an explicit manifest for development builds
py -3 .\tools\openxr_runtime.py --runtime custom --manifest "C:\Runtime\questbridge_openxr_runtime.json" --app "C:\Path\To\OpenXRApp.exe"
```

## Limitations

This is **not** an OpenXR runtime or compositor. It cannot turn QuestBridge into the runtime for arbitrary games, it cannot force Roblox DOORS to implement native VR, and it cannot capture an application's stereo swapchains. A compatible app still depends on the selected runtime and the capabilities of the game. QuestBridge does not currently implement its own Windows runtime DLL, D3D11/D3D12 swapchain/compositor path, or PCVR frame-streaming integration.

Actual use of the per-process override needs to be verified on Windows with Meta Quest Link and a known compatible OpenXR app. The included unit tests validate manifest discovery, selection precedence, and child-process environment isolation; they do not establish that a given game works with Meta runtime.
