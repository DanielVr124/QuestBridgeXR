#pragma once

#include <cstddef>
#include <cstdint>

namespace qbx {

// Wire format: little-endian fields. Windows x64 and Quest ARM64 are little-endian.
// Keep packets small enough to avoid IP fragmentation on a typical local network.
inline constexpr std::uint32_t kMagic = 0x31584251u; // bytes: 'Q' 'B' 'X' '1'
inline constexpr std::uint16_t kProtocolVersion = 1;
inline constexpr std::size_t kMaxUdpPayload = 1200;
inline constexpr std::uint16_t kDefaultPort = 9944;

enum class PacketType : std::uint16_t {
    Hello = 1,
    HelloAck = 2,
    Tracking = 3,
    Controller = 4,
    Heartbeat = 5,
    FrameChunk = 6, // encoded video access-unit chunk
    Goodbye = 7,
    VideoConfig = 8, // host/test sender announces dimensions and stereo layout
};

#pragma pack(push, 1)
struct PacketHeader {
    std::uint32_t magic;
    std::uint16_t version;
    std::uint16_t type;
    std::uint32_t sequence;
    std::uint32_t payloadBytes;
};

struct HelloPayload {
    std::uint32_t clientVersion;
    std::uint32_t capabilities;
    char deviceName[32];
};

struct HelloAckPayload {
    std::uint32_t serverVersion;
    std::uint32_t acceptedCapabilities;
    std::uint16_t streamPort;
    std::uint16_t reserved;
};

// Positions are metres in the runtime's chosen tracking origin. Orientation is quaternion x,y,z,w.
// A production implementation must define origin, handedness, timestamp clock and prediction rules.
struct TrackingPayload {
    std::uint64_t timestampNs;
    float headPosition[3];
    float headOrientation[4];
    float linearVelocity[3];
    float angularVelocity[3];
    std::uint32_t trackingFlags; // bit 0 = position valid; bit 1 = orientation valid
};

struct ControllerPayload {
    std::uint64_t timestampNs;
    std::uint8_t hand; // 0 = left, 1 = right
    std::uint8_t trackingFlags;
    std::uint16_t reserved;
    std::uint32_t buttons; // project-defined bit mask; see docs/protocol.md
    float trigger;         // 0..1
    float grip;            // 0..1
    float thumbstick[2];   // -1..1
    float position[3];
    float orientation[4];  // x,y,z,w
};

struct HeartbeatPayload {
    std::uint64_t timestampNs;
};

// Sent before FrameChunk packets by the PC sender. layout: 0 = mono (same image per eye), 1 = side-by-side stereo.
struct VideoConfigPayload {
    std::uint32_t width;       // encoded frame width (full frame, including both eyes for SBS)
    std::uint32_t height;
    std::uint32_t framesPerSecond;
    std::uint32_t bitrateBitsPerSecond;
    std::uint16_t codec;       // 1 = H.264, 2 = HEVC
    std::uint16_t layout;      // 0 = mono, 1 = side-by-side stereo
};

// Reserved frame packet payload header. One PacketHeader + FrameChunkPrefix + encoded bytes per UDP datagram.
// Use frame IDs and chunk indices to reassemble; never wait indefinitely for missing chunks.
struct FrameChunkPrefix {
    std::uint64_t timestampNs;
    std::uint32_t frameId;
    std::uint32_t totalFrameBytes;
    std::uint16_t chunkIndex;
    std::uint16_t chunkCount;
    std::uint16_t codec; // 1 = H.264, 2 = HEVC
    std::uint16_t flags; // bit 0 = keyframe
};
#pragma pack(pop)

static_assert(sizeof(PacketHeader) == 16);
static_assert(sizeof(TrackingPayload) == 64);
static_assert(sizeof(ControllerPayload) == 60);
static_assert(sizeof(VideoConfigPayload) == 20);

} // namespace qbx
