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

`controller_pose` is `grip` or `aim`. `reference_space` is `local` or `stage`; STAGE is selected only if the active Quest OpenXR runtime reports it as supported. `brightness` is bounded to -0.5..0.5 and `contrast` to 0.25..2.5. Positions are scaled in the outgoing tracking telemetry, not in the local compositor's pose used to render the Quest app.

## Quest room controls

With the default room controls enabled:

- **Left thumbstick:** walk around the room.
- **Right thumbstick:** turn; snap-turn is enabled by default.
- **Click the right thumbstick (R3):** cycle to the next environment. There are 208 scenes, so this currently advances one at a time and wraps around at the end.
- **Press the three-lines Menu button on the left controller:** open or close the monitor's QuestBridge Control Center. Some Quest runtimes reserve this button for the system menu; right B remains a compatibility fallback. The menu replaces the live PC image while open and reveals it again when closed.
- **Point the right controller at the monitor:** a continuous blue laser follows the controller's aim pose; squeeze the right trigger to select menu tabs and actions. The menu includes Home, Environments, Display Output, Comfort, and Help.
- **Click the left thumbstick (L3):** cycle the desktop display between the monitor, TV, and both.
- **Press X on the left controller:** toggle the seated-height mode. Moving with the left thumbstick automatically stands you back up.

If a control does not respond, check that `room_controls_enabled=true`, `room_movement_enabled=true`, and `sitting_toggle_enabled=true` are present in the active `config.ini`. Controller input forwarding and room navigation use the same OpenXR actions.

## Diagnostics

Use `adb logcat -s QuestBridgeXR` to see OpenXR initialization, network connection, video configuration, and decoder messages. The client app running locally on Quest does not itself turn the Windows host into a PC OpenXR runtime. The Windows host can optionally forward accepted tracking/controller packets to `127.0.0.1:9945` for diagnostic/future runtime consumers; that feed is configured in the root JSON profile and is not used by a finished compositor yet.
