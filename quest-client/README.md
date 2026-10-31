# QuestBridge XR native Quest client

This Android NativeActivity is a source-level bring-up client using the Khronos OpenXR Android loader, OpenGL ES stereo swapchains, Quest OpenXR tracking/actions, and a UDP control/video transport. It has not yet been compiled or validated on real Quest hardware in this development environment (source version 0.3.1).

## Build requirements

- Android Studio and JDK 17
- Android SDK Platform 35
- Android NDK `27.2.12479018`
- CMake `3.22.1`
- Quest headset with Developer Mode / USB debugging for installation tests

Open this directory in Android Studio, sync Gradle, build `app` debug, then install `app/build/outputs/apk/debug/app-debug.apk` with ADB. Run the app once to create its private external-files directory.

## Configuration

Use QuestBridge Studio at the repository root (`QuestBridgeStudio.bat`) to generate `config.ini`. Install it using the Studio ADB button or copy it to:

`/sdcard/Android/data/com.questbridge.xr/files/config.ini`

The app currently reads these values (unknown keys are ignored):

```ini
host_ip=192.168.1.100
host_port=9944
local_port=9944
tracking_enabled=true
controllers_enabled=true
tracking_hz=72
position_scale=1.0
thumbstick_deadzone=0.12
invert_thumbstick_x=false
invert_thumbstick_y=false
swap_face_buttons=false
controller_pose=grip
reference_space=local
brightness=0.0
contrast=1.0
swap_eyes=false
max_frame_mib=16
incomplete_frame_timeout_ms=120
```

`controller_pose` is `grip` or `aim`. `reference_space` is `local` or `stage`; STAGE is selected only if the active Quest OpenXR runtime reports it as supported.

## Room hardware controls

With the room menu closed, point the right controller at the wall rocker beside the entry and squeeze the trigger to toggle the main room lights. The recessed RGB strips remain on so the room can stay in its neon gaming look. Point down at the six-button remote on the right bedside table and squeeze a color button to change the ceiling coves, wall strips, bedside accents, and colored light bounce together. These interactions use the existing right-hand pose and trigger and do not replace joystick movement, turning, or menu bindings. `brightness` is bounded to -0.5..0.5 and `contrast` to 0.25..2.5. Positions are scaled in the outgoing tracking telemetry, not in the local compositor's pose used to render the Quest app.

## Diagnostics

Use `adb logcat -s QuestBridgeXR` to see OpenXR initialization, network connection, video configuration, and decoder messages. The client app running locally on Quest does not itself turn the Windows host into a PC OpenXR runtime. The Windows host can optionally forward accepted tracking/controller packets to `127.0.0.1:9945` for diagnostic/future runtime consumers; that feed is configured in the root JSON profile and is not used by a finished compositor yet.
