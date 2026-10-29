# QBX Protocol v1

All multibyte fields are little-endian. Windows x64 and Quest ARM64 are little-endian. `PacketHeader` is 16 bytes. UDP datagrams must not exceed 1200 bytes in the current sender/receiver implementation.

| Type | Value | Payload | Purpose |
|---|---:|---|---|
| HELLO | 1 | `HelloPayload` | Client announces itself and capabilities |
| HELLO_ACK | 2 | `HelloAckPayload` | Host confirms protocol and listening port |
| TRACKING | 3 | `TrackingPayload` | HMD pose/velocity and validity flags |
| CONTROLLER | 4 | `ControllerPayload` | One controller pose, buttons, trigger, grip, stick |
| HEARTBEAT | 5 | `HeartbeatPayload` | Connection liveness |
| FRAME_CHUNK | 6 | `FrameChunkPrefix` + encoded bytes | One chunk from an encoded video access unit |
| GOODBYE | 7 | empty | Graceful disconnect |
| VIDEO_CONFIG | 8 | `VideoConfigPayload` | Encoded dimensions, rate, codec and mono/SBS layout |

## HELLO capability bits

- bit 0: tracking payloads
- bit 1: controller payloads
- bit 2: H.264 decode
- bit 3: HEVC decode

The current host echoes the capability mask; production negotiation must intersect it with server capabilities instead of echoing blindly.

## Controller button bits

- bit 0: primary face button (A on right / X on left)
- bit 1: secondary face button (B on right / Y on left)
- bit 2: thumbstick click
- bit 3: left menu click (no system/home button mapping)
- bit 4: squeeze/grip click
- bit 5: trigger click
- bit 6: optional thumbrest/touch sensor (not currently populated)

## Coordinates and timing

- Position units: metres.
- Orientation: quaternion `(x, y, z, w)`.
- Space: Quest OpenXR `LOCAL` reference space; recenter/origin changes are supplied by the Quest runtime.
- `timestampNs` in tracking payloads is currently a monotonic diagnostic timestamp, not synchronized between PC and headset. HMD velocity fields are reserved but currently sent as zero. Video presentation timestamps now use the sender's monotonic frame timestamp rather than `frameId`, so restarting the sender does not inherently rewind decoder PTS. Full clock synchronization, pose prediction, velocity sampling and stale-pose rejection remain production work.
- The Windows host can optionally forward accepted TRACKING and CONTROLLER datagrams to `127.0.0.1:<telemetry_port>` (default 9945; set to 0 to disable). Only packets from the most recent HELLO peer endpoint are forwarded. This localhost feed is consumed by the experimental Windows OpenXR runtime to provide HMD/controller poses and input actions.
- Tracking flags: bit 0 position valid, bit 1 orientation valid.
- Controller tracking flags use the same bit assignment.

## Video config

`VideoConfigPayload` is 20 bytes: width, height, frame rate, bitrate, codec and layout. Codec 1 is H.264/AVC; codec 2 is HEVC. Layout 0 means one mono image duplicated in both eyes; layout 1 means side-by-side stereo with the left eye in the left half and the right eye in the right half.

## Video chunking

`FrameChunkPrefix` is 24 bytes and contains a monotonic sender timestamp, frame ID, complete frame byte count, chunk index/count, codec and flags. Flag bit 0 marks a keyframe. The current sender uses 1160 encoded bytes per chunk, so `PacketHeader + FrameChunkPrefix + chunk` is 1200 bytes. The receiver reassembles one in-flight frame, rejects oversized/inconsistent frames, expires incomplete frames quickly, and resumes at the next complete frame. It permits up to 32768 chunks per frame (the current configured 32 MiB limit fits below this at 1160 bytes per chunk). A quiet gap followed by `VIDEO_CONFIG` resets the frame-ID tracker so a restarted test sender can begin at frame ID 1 again. A production transport should add feedback-driven keyframe requests and recovery; the test sender repeats SPS/PPS on keyframes and emits regular IDRs.

## Compatibility note

The wire protocol is project-specific and not a standard OpenXR transport. It does not define the Windows OpenXR runtime ABI. The version should be bumped if a breaking binary-layout change is introduced.
