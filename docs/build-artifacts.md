# Building ready-to-run artifacts

## What can be built automatically

The repository contains `.github/workflows/build-release.yml`. On GitHub, it builds two target-specific products on hosted runners and combines them into a portable ZIP plus a Windows installer:

- **Windows portable package**: `QuestBridgeStudio.exe`, the C++ UDP host, bundled diagnostic/streaming helper executables, configurations, room assets, Quest client source, and docs.
- **Quest debug APK**: the Android NativeActivity/OpenXR client built for `arm64-v8a`.
- **Combined ZIP**: both products together, plus the Quest APK in `QuestClient/QuestBridgeXR-Quest-debug.apk`.

The workflow can be run manually with **Actions → QuestBridge XR Build → Run workflow** after the project is pushed to a GitHub repository. It also runs on pushes and pull requests to `main`/`master`. Download the `QuestBridgeXR-Portable` artifact from a completed run, or push a `v*` tag to publish the ZIP, SHA-256 checksum, and Windows Setup EXE as a GitHub Release. The workflow is a build recipe; it has not run from this environment because this environment has neither Windows/MSVC nor the Android SDK/NDK and cannot download missing toolchains. The new Inno Setup script is therefore not yet validated by a Windows install/uninstall test.

## Downloading a ready-made package from a GitHub Release

The workflow is configured to run on pushes to `main`/`master`, on pull requests, and on tags matching `v*`. Once the source is in a GitHub repository and the Actions workflow passes, pushing a tag such as `v0.8.1` publishes `QuestBridgeXR-0.8.1-Portable.zip`, its `.sha256` checksum, and `QuestBridgeXR-0.8.1-Setup.exe` as GitHub Release assets. The ZIP/installer contain standalone Studio/host/helper executables and the Quest debug APK. The installer is per-user and does not require administrator privileges; it preserves the active config and saved profiles across upgrades/uninstall. It still requires a developer-enabled Quest and USB debugging to install the APK, and FFmpeg is not bundled.

## Building locally on Windows

1. Install Visual Studio 2022 Build Tools with Desktop development with C++, CMake 3.24+, and Python 3.11+.
2. Double-click `Build-QuestBridgeXR.bat`, or run `\.\scripts\build-all.ps1` from PowerShell in the project root.
3. The script builds the Windows host and experimental D3D11 OpenXR runtime, makes Studio and helper tools standalone, and creates `dist\QuestBridgeXR-0.8.1-Portable.zip` plus a SHA-256 checksum. The runtime build fetches the pinned Khronos OpenXR-SDK dependency. If Gradle and the Android SDK/NDK are installed, it also builds and includes the Quest APK. Without those Android tools, the Windows package records that the APK was omitted.
4. Extract the ZIP, then start `QuestBridgeStudio.exe`. Keep the extracted files together; configuration, assets, helper EXEs, and the optional Quest APK are relative to that folder.

For streaming, install an FFmpeg build that contains `libx264` (and `gdigrab` for Windows desktop capture). FFmpeg is not bundled by this project.

## Installing the Quest debug APK

The GitHub workflow artifact contains `QuestClient/QuestBridgeXR-Quest-debug.apk`. Enable Developer Mode and USB debugging, connect the Quest over USB, then run:

```powershell
adb install -r .\QuestClient\QuestBridgeXR-Quest-debug.apk
```

Run QuestBridge XR once to create its app-private configuration directory, then use Studio's **Install Quest INI via ADB** action.

## OpenXR / PCVR compatibility caveat

This build includes the first experimental QuestBridge-owned Windows OpenXR runtime/compositor source and, when the Windows build succeeds, its D3D11 DLL and manifest. It is not Khronos-conformance-tested, has limited extension/API support, and must only be selected with Studio's per-process `XR_RUNTIME_JSON` option. It mirrors submitted projection images into a window, which the FFmpeg sender captures to stream to Quest; the capture/encode path adds latency and does not perform timewarp. Keep the known-good system runtime unchanged, and use Meta Quest Link for compatible applications if the experimental runtime fails.
## Uploading the project so Actions can build it

1. Create a new repository on GitHub (private is fine); do not initialize it with a README/license because this folder already has those files.
2. Extract the source ZIP, open a terminal in the extracted `QuestBridgeXR` folder, and run these commands (replace the example remote with your repository URL):

```powershell
git init
git add .
git commit -m "QuestBridge XR 0.8.1 source"
git branch -M main
git remote add origin https://github.com/YOUR-ACCOUNT/QuestBridgeXR.git
git push -u origin main
```

3. Open the repository's **Actions** tab and select **QuestBridge XR Build**. The workflow starts on the push; after it succeeds, open the run and download the `QuestBridgeXR-Portable` artifact. To publish a permanent release, run `git tag v0.8.1`, then `git push origin v0.8.1`; the tagged workflow publishes the portable ZIP, checksum, and Windows Setup EXE under **Releases**.

Git and a GitHub account are required for this route. A connected GitHub integration can also help publish the source and inspect the resulting build workflow, but it must be connected by the account owner first.

