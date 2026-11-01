# Quest USB connection checks

QuestBridge Studio's **Connect** tab separates four different questions:

1. **ADB tools** — whether Windows can run Android Debug Bridge (adb.exe).
2. **USB authorization** — whether a data-capable cable is connected and the Quest has approved this PC's USB-debugging key.
3. **Quest response / app state** — whether the headset answers shell queries, whether QuestBridge XR is installed, and whether its process is running.
4. **Quest-to-PC / video evidence** — when QuestBridge is installed, Studio reads the retained QuestBridgeXR Android log entries. A "Windows host accepted connection" entry confirms that the Quest app received the Windows host's QBX HELLO_ACK at least once. "Received video config" or "Video decoder configured" confirms the sender's stream setup reached the Quest decoder.

The log-based checks are evidence from the latest retained Android log buffer, not a live frame-rate or image-quality measurement. Refresh after opening QuestBridge on the headset to check the current session.

## Expected USB setup

- Use a USB cable that supports data, not charge-only.
- Unlock the headset and enable Developer Mode / USB debugging.
- Accept the **Allow USB debugging** prompt in the headset.
- Keep only one USB Android device connected when installing the APK or sending config.ini.
- If Studio cannot find ADB, install Android Platform-Tools or choose the adb.exe inside its platform-tools folder using **Locate adb.exe**.

The portable Windows builder copies adb.exe and its Windows API DLLs into platform-tools/ when Platform-Tools are available on the build machine. Otherwise, the packaged UI can discover the normal Android SDK locations or let you select adb.exe manually.

## Wired USB streaming

Studio now has a **Start Wired USB Streaming** action on the Connect tab. It uses a bidirectional TCP connection carried over ADB reverse:

1. Studio confirms exactly one authorized USB Quest is attached and QuestBridge XR is installed.
2. Studio runs `adb -s <serial> reverse tcp:9946 tcp:9946`, mapping the Quest's loopback TCP port to the Windows loopback listener.
3. Studio exports and pushes `config.ini` with `transport_mode=usb` and `usb_port=9946`, starts the existing Windows host, and starts the video sender in USB mode.
4. Studio restarts QuestBridge XR. The Quest app connects to `127.0.0.1:9946`; this is routed through the USB cable to the PC.
5. Existing QBX packets are length-prefixed on the TCP stream. Tracking, controller, heartbeat, and handshake packets travel Quest → PC. The sender returns VIDEO_CONFIG and encoded FRAME_CHUNK packets PC → Quest over that same connection.
6. The Windows sender relays Quest control/handshake packets to the existing host on localhost UDP. This keeps the host's existing tracking/input path intact while removing Wi-Fi from the Quest stream path.

The TCP listener binds only to `127.0.0.1`; it is not exposed as a LAN service. The existing UDP/Wi-Fi mode remains the default and is unchanged. To return to Wi-Fi mode, send the normal settings from Studio (which exports `transport_mode=udp`) and restart the Quest app.

### What the status means

- **USB ready** means Windows can communicate with the headset using ADB. It does not by itself prove streaming.
- **Wired USB stream handshake accepted** means the Quest received the Windows host's QBX HELLO_ACK through the USB tunnel.
- **Received video config / Video decoder configured** means the video stream reached and configured the Quest decoder.
- A configured decoder does not guarantee smooth playback, low latency, or that every frame was displayed. Those require a physical headset test.

### Requirements and limitations

- Use a data-capable USB cable, unlock the Quest, enable Developer Mode / USB debugging, and approve the PC's debugging key.
- Keep exactly one authorized USB Quest connected while starting wired mode.
- FFmpeg with `libx264` must be available to the packaged video sender.
- This build's video source options remain the existing test pattern, desktop preview, and compositor mirror. USB changes the transport, not the source or compositor implementation.
- The USB session is intentionally separate from the UDP sender. Stop the current streamer before starting wired mode.
- ADB reverse/tcp transport is reliable and ordered, unlike UDP; the TCP framing code handles split and coalesced reads. It can still build queueing latency if the PC, cable, or decoder cannot keep up.

The Quest-side TCP transport, Windows sender, and ADB orchestration are covered by automated source/build tests. Actual Quest 3 throughput, decoder behavior, cable quality, and end-to-end latency still require validation on the headset.
