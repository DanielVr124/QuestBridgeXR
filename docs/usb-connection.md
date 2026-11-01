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

## USB is not yet wired video transport

The existing QuestBridge stream protocol uses UDP for the headset's tracking/controller packets and PC-to-Quest video. ADB-over-USB currently supports device detection, installation, diagnostics, and settings transfer; it does **not** tunnel the live UDP video/tracking stream over the cable. The PC and Quest must still be on a network that can carry the configured UDP traffic.

Therefore:
- **USB ready** means Windows can communicate with the headset using ADB.
- **Quest → PC handshake seen** means the Quest app reached the Windows host over the configured network at least once.
- **Video setup seen** means stream configuration reached the Quest decoder; it does not guarantee smooth playback or prove every frame rendered.

If the USB check is green but the handshake is missing, check the PC IP, host UDP port, Windows Firewall, and same-LAN connection. If the handshake is present but video setup is missing, start the video sender and check its configured Quest IP and UDP destination port.
