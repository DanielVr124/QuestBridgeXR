#define XR_USE_PLATFORM_ANDROID
#define XR_USE_GRAPHICS_API_OPENGL_ES
#include <jni.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <android/log.h>
#include <android_native_app_glue.h>
#include <android/native_window.h>
#include <GLES3/gl3.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cctype>

#include "qbx/protocol.hpp"
#include "video_decoder.h"
#include "room_renderer.h"

#define QBX_LOG_TAG "QuestBridgeXR"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, QBX_LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, QBX_LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, QBX_LOG_TAG, __VA_ARGS__)

namespace {
using Clock = std::chrono::steady_clock;
constexpr uint32_t kCapabilitiesTracking = 1u << 0;
constexpr uint32_t kCapabilitiesControllers = 1u << 1;
constexpr uint32_t kCapabilitiesH264 = 1u << 2;
constexpr uint32_t kCapabilitiesHevc = 1u << 3;
constexpr uint16_t kCodecH264 = 1;
constexpr uint16_t kCodecHevc = 2;

uint64_t nowNs() {
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
        Clock::now().time_since_epoch()).count());
}

bool xrOk(XrResult result, const char* operation) {
    if (XR_SUCCEEDED(result)) return true;
    LOGE("%s failed with OpenXR result %d", operation, static_cast<int>(result));
    return false;
}

struct ClientConfig {
    std::string hostIp = "192.168.1.100";
    uint16_t hostPort = qbx::kDefaultPort;
    uint16_t localPort = qbx::kDefaultPort;
    bool trackingEnabled = true;
    bool controllersEnabled = true;
    bool invertThumbstickX = false;
    bool invertThumbstickY = false;
    bool swapFaceButtons = false;
    bool swapHands = false;
    bool swapEyes = false;
    bool flipHorizontal = false;
    bool flipVertical = false;
    std::string controllerPose = "grip";
    std::string referenceSpace = "local";
    std::string aspectMode = "stretch";
    uint16_t trackingHz = 72;
    float positionScale = 1.0f;
    float thumbstickDeadzone = 0.12f;
    float triggerDeadzone = 0.02f;
    float triggerCurve = 1.0f;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    float offsetZ = 0.0f;
    float brightness = 0.0f;
    float contrast = 1.0f;
    float saturation = 1.0f;
    float gamma = 1.0f;
    uint32_t maxFrameMiB = 16;
    uint32_t incompleteFrameTimeoutMs = 120;
    uint32_t receiveBufferKiB = 4096;
    RoomSettings room{};
    bool roomMovementEnabled = true;
};

bool parseBool(const std::string& text, bool fallback) {
    if (text == "true" || text == "1" || text == "yes" || text == "on") return true;
    if (text == "false" || text == "0" || text == "no" || text == "off") return false;
    return fallback;
}

std::string trim(const std::string& text) {
    const auto first = std::find_if_not(text.begin(), text.end(), [](unsigned char c) { return std::isspace(c) != 0; });
    const auto last = std::find_if_not(text.rbegin(), text.rend(), [](unsigned char c) { return std::isspace(c) != 0; }).base();
    if (first >= last) return {};
    return std::string(first, last);
}

float parseFloatInRange(const std::string& text, float fallback, float minimum, float maximum) {
    char* end = nullptr;
    const float value = std::strtof(text.c_str(), &end);
    if (end == text.c_str() || *end != '\0' || !std::isfinite(value) || value < minimum || value > maximum) return fallback;
    return value;
}

ClientConfig loadConfig(android_app* app) {
    ClientConfig cfg;
    const char* root = app && app->activity ? app->activity->externalDataPath : nullptr;
    if (!root || !*root) {
        LOGW("Android external files path unavailable; using default host IP %s", cfg.hostIp.c_str());
        return cfg;
    }
    const std::string path = std::string(root) + "/config.ini";
    std::ifstream input(path);
    if (!input.good()) {
        std::ofstream out(path);
        out << "# Edit host_ip to the Windows PC's LAN IPv4 address.\n"
            << "host_ip=192.168.1.100\n"
            << "host_port=9944\n"
            << "local_port=9944\n"
            << "tracking_enabled=true\n"
            << "controllers_enabled=true\n"
            << "tracking_hz=72\n"
            << "position_scale=1.0\n"
            << "thumbstick_deadzone=0.12\n"
            << "invert_thumbstick_x=false\n"
            << "invert_thumbstick_y=false\n"
            << "swap_face_buttons=false\n"
            << "swap_hands=false\n"
            << "controller_pose=grip\n"
            << "reference_space=local\n"
            << "trigger_deadzone=0.02\n"
            << "trigger_curve=1.0\n"
            << "tracking_offset_x=0.0\n"
            << "tracking_offset_y=0.0\n"
            << "tracking_offset_z=0.0\n"
            << "brightness=0.0\n"
            << "contrast=1.0\n"
            << "saturation=1.0\n"
            << "gamma=1.0\n"
            << "aspect_mode=stretch\n"
            << "flip_horizontal=false\n"
            << "flip_vertical=false\n"
            << "swap_eyes=false\n"
            << "max_frame_mib=16\n"
            << "incomplete_frame_timeout_ms=120\n"
            << "quest_receive_buffer_kib=4096\n"
            << "environment=blossom\n"
            << "room_size=medium\n"
            << "screen_destination=monitor\n"
            << "monitor_count=1\n"
            << "show_tv=true\n"
            << "show_couch=true\n"
            << "show_chair=true\n"
            << "room_controls_enabled=true\n"
            << "sitting_toggle_enabled=true\n"
            << "sitting_head_drop_m=0.42\n"
            << "room_movement_enabled=true\n"
            << "movement_speed=1.35\n"
            << "smooth_turn=false\n"
            << "turn_speed=1.8\n"
            << "snap_turn_degrees=30\n"
            << "comfort_vignette=true\n"
            << "room_floor_offset_y=-1.6\n";
        LOGI("Created config file at %s; change host_ip to your PC's LAN IP", path.c_str());
        return cfg;
    }
    std::string line;
    while (std::getline(input, line)) {
        line = trim(line); // Also removes CR from Windows CRLF INI files.
        if (line.empty() || line[0] == '#') continue;
        const auto split = line.find('=');
        if (split == std::string::npos) continue;
        const std::string key = trim(line.substr(0, split));
        const std::string value = trim(line.substr(split + 1));
        if (key == "host_ip" && !value.empty()) cfg.hostIp = value;
        else if (key == "host_port") {
            const int v = std::atoi(value.c_str());
            if (v > 0 && v <= 65535) cfg.hostPort = static_cast<uint16_t>(v);
        } else if (key == "local_port") {
            const int v = std::atoi(value.c_str());
            if (v > 0 && v <= 65535) cfg.localPort = static_cast<uint16_t>(v);
        } else if (key == "tracking_enabled") cfg.trackingEnabled = parseBool(value, cfg.trackingEnabled);
        else if (key == "controllers_enabled") cfg.controllersEnabled = parseBool(value, cfg.controllersEnabled);
        else if (key == "tracking_hz") {
            const int v = std::atoi(value.c_str());
            if (v >= 10 && v <= 240) cfg.trackingHz = static_cast<uint16_t>(v);
        } else if (key == "position_scale") cfg.positionScale = parseFloatInRange(value, cfg.positionScale, 0.1f, 5.0f);
        else if (key == "thumbstick_deadzone") cfg.thumbstickDeadzone = parseFloatInRange(value, cfg.thumbstickDeadzone, 0.0f, 0.9f);
        else if (key == "invert_thumbstick_x") cfg.invertThumbstickX = parseBool(value, cfg.invertThumbstickX);
        else if (key == "invert_thumbstick_y") cfg.invertThumbstickY = parseBool(value, cfg.invertThumbstickY);
        else if (key == "swap_face_buttons") cfg.swapFaceButtons = parseBool(value, cfg.swapFaceButtons);
        else if (key == "swap_hands") cfg.swapHands = parseBool(value, cfg.swapHands);
        else if (key == "controller_pose" && (value == "grip" || value == "aim")) cfg.controllerPose = value;
        else if (key == "reference_space" && (value == "local" || value == "stage")) cfg.referenceSpace = value;
        else if (key == "trigger_deadzone") cfg.triggerDeadzone = parseFloatInRange(value, cfg.triggerDeadzone, 0.0f, 0.5f);
        else if (key == "trigger_curve") cfg.triggerCurve = parseFloatInRange(value, cfg.triggerCurve, 0.25f, 3.0f);
        else if (key == "tracking_offset_x") cfg.offsetX = parseFloatInRange(value, cfg.offsetX, -2.0f, 2.0f);
        else if (key == "tracking_offset_y") cfg.offsetY = parseFloatInRange(value, cfg.offsetY, -2.0f, 2.0f);
        else if (key == "tracking_offset_z") cfg.offsetZ = parseFloatInRange(value, cfg.offsetZ, -2.0f, 2.0f);
        else if (key == "brightness") cfg.brightness = parseFloatInRange(value, cfg.brightness, -0.5f, 0.5f);
        else if (key == "contrast") cfg.contrast = parseFloatInRange(value, cfg.contrast, 0.25f, 2.5f);
        else if (key == "saturation") cfg.saturation = parseFloatInRange(value, cfg.saturation, 0.0f, 2.0f);
        else if (key == "gamma") cfg.gamma = parseFloatInRange(value, cfg.gamma, 0.5f, 2.5f);
        else if (key == "aspect_mode" && (value == "stretch" || value == "fit" || value == "crop")) cfg.aspectMode = value;
        else if (key == "flip_horizontal") cfg.flipHorizontal = parseBool(value, cfg.flipHorizontal);
        else if (key == "flip_vertical") cfg.flipVertical = parseBool(value, cfg.flipVertical);
        else if (key == "swap_eyes") cfg.swapEyes = parseBool(value, cfg.swapEyes);
        else if (key == "max_frame_mib") {
            const int v = std::atoi(value.c_str());
            if (v >= 1 && v <= 32) cfg.maxFrameMiB = static_cast<uint32_t>(v);
        } else if (key == "incomplete_frame_timeout_ms") {
            const int v = std::atoi(value.c_str());
            if (v >= 20 && v <= 1000) cfg.incompleteFrameTimeoutMs = static_cast<uint32_t>(v);
        } else if (key == "quest_receive_buffer_kib") {
            const int v = std::atoi(value.c_str());
            if (v >= 256 && v <= 16384) cfg.receiveBufferKiB = static_cast<uint32_t>(v);
        } else if (key == "environment" && (value == "blossom" || value == "night_city" || value == "golden_city" || value == "aurora" || value == "cyberpunk" || value == "sunset_beach" || value == "snowy_mountains" || value == "enchanted_forest" || value == "desert_dunes" || value == "ocean_cliffs" || value == "space_nebula" || value == "lunar_base" || value == "volcanic" || value == "underwater" || value == "autumn_forest" || value == "tropical_island" || value == "candy_land" || value == "steampunk_city" || value == "rainy_tokyo" || value == "sakura_night" || value == "alpine_lake" || value == "emerald_valley" || value == "wasteland" || value == "midnight_clouds" || value == "crystal_cavern" || value == "lavender_fields" || value == "firefly_swamp" || value == "polar_station")) cfg.room.environment = value;
        else if (key == "room_size" && (value == "small" || value == "medium" || value == "large")) cfg.room.roomSize = value;
        else if (key == "screen_destination" && (value == "monitor" || value == "tv" || value == "both")) cfg.room.screenDestination = value;
        else if (key == "monitor_count") { const int v = std::atoi(value.c_str()); if (v >= 1 && v <= 3) cfg.room.monitorCount = v; }
        else if (key == "show_tv") cfg.room.showTv = parseBool(value, cfg.room.showTv);
        else if (key == "show_couch") cfg.room.showCouch = parseBool(value, cfg.room.showCouch);
        else if (key == "show_chair") cfg.room.showChair = parseBool(value, cfg.room.showChair);
        else if (key == "room_controls_enabled") cfg.room.roomControlsEnabled = parseBool(value, cfg.room.roomControlsEnabled);
        else if (key == "sitting_toggle_enabled") cfg.room.sittingToggleEnabled = parseBool(value, cfg.room.sittingToggleEnabled);
        else if (key == "sitting_head_drop_m") cfg.room.sittingHeadDropM = parseFloatInRange(value, cfg.room.sittingHeadDropM, 0.1f, 0.75f);
        else if (key == "room_movement_enabled") cfg.roomMovementEnabled = parseBool(value, cfg.roomMovementEnabled);
        else if (key == "movement_speed") cfg.room.movementSpeed = parseFloatInRange(value, cfg.room.movementSpeed, 0.1f, 4.0f);
        else if (key == "smooth_turn") cfg.room.smoothTurn = parseBool(value, cfg.room.smoothTurn);
        else if (key == "turn_speed") cfg.room.turnSpeed = parseFloatInRange(value, cfg.room.turnSpeed, 0.2f, 3.5f);
        else if (key == "snap_turn_degrees") cfg.room.snapTurnDegrees = parseFloatInRange(value, cfg.room.snapTurnDegrees, 15.0f, 60.0f);
        else if (key == "comfort_vignette") cfg.room.comfortVignette = parseBool(value, cfg.room.comfortVignette);
        else if (key == "room_floor_offset_y") cfg.room.floorOffsetY = parseFloatInRange(value, cfg.room.floorOffsetY, -2.5f, 0.0f);
    }
    LOGI("Config: host=%s:%u local_udp=%u room=%s/%s", cfg.hostIp.c_str(), cfg.hostPort, cfg.localPort,
         cfg.room.environment.c_str(), cfg.room.roomSize.c_str());
    return cfg;
}

#pragma pack(push, 1)
struct WireFrameChunkPrefix {
    uint64_t timestampNs;
    uint32_t frameId;
    uint32_t totalFrameBytes;
    uint16_t chunkIndex;
    uint16_t chunkCount;
    uint16_t codec;
    uint16_t flags;
};
#pragma pack(pop)
static_assert(sizeof(WireFrameChunkPrefix) == sizeof(qbx::FrameChunkPrefix));

struct FrameAssembly {
    uint32_t frameId = 0;
    uint32_t totalBytes = 0;
    uint16_t chunkCount = 0;
    uint16_t codec = kCodecH264;
    uint16_t flags = 0;
    uint64_t timestampNs = 0;
    Clock::time_point lastUpdate{};
    size_t bytesReceived = 0;
    size_t chunksReceived = 0;
    std::vector<std::vector<uint8_t>> chunks;
    std::vector<uint8_t> received;
    void reset() { *this = FrameAssembly{}; }
};

class NetworkClient {
public:
    bool start(const ClientConfig& config) {
        config_ = config;
        maxFrameBytes_ = static_cast<size_t>(config.maxFrameMiB) * 1024u * 1024u;
        incompleteFrameTimeoutMs_ = config.incompleteFrameTimeoutMs;
        socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (socket_ < 0) {
            LOGE("UDP socket() failed: %d", errno);
            return false;
        }
        int reuse = 1;
        setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
        const int receiveBufferBytes = static_cast<int>(config.receiveBufferKiB * 1024u);
        if (setsockopt(socket_, SOL_SOCKET, SO_RCVBUF, &receiveBufferBytes, sizeof(receiveBufferBytes)) != 0) {
            LOGW("Could not set UDP receive buffer to %u KiB (errno=%d); continuing with OS default",
                 config.receiveBufferKiB, errno);
        }
        sockaddr_in local{};
        local.sin_family = AF_INET;
        local.sin_addr.s_addr = htonl(INADDR_ANY);
        local.sin_port = htons(config.localPort);
        if (bind(socket_, reinterpret_cast<sockaddr*>(&local), sizeof(local)) != 0) {
            LOGE("Cannot bind UDP local port %u (errno=%d). Set local_port in config.ini.", config.localPort, errno);
            close(socket_);
            socket_ = -1;
            return false;
        }
        const int currentFlags = fcntl(socket_, F_GETFL, 0);
        fcntl(socket_, F_SETFL, currentFlags | O_NONBLOCK);
        std::memset(&host_, 0, sizeof(host_));
        host_.sin_family = AF_INET;
        host_.sin_port = htons(config.hostPort);
        if (inet_pton(AF_INET, config.hostIp.c_str(), &host_.sin_addr) != 1) {
            LOGE("Invalid host_ip in config.ini: %s", config.hostIp.c_str());
            return false;
        }
        sendHello();
        nextHello_ = Clock::now() + std::chrono::seconds(1);
        nextHeartbeat_ = Clock::now() + std::chrono::seconds(1);
        LOGI("UDP client ready; HELLO sent to %s:%u; receive buffer request=%u KiB",
             config.hostIp.c_str(), config.hostPort, config.receiveBufferKiB);
        return true;
    }

    ~NetworkClient() {
        if (socket_ >= 0) {
            if (connected_) sendPacket(qbx::PacketType::Goodbye, nullptr, 0);
            close(socket_);
        }
    }

    bool connected() const { return connected_; }

    void tick(VideoDecoder& video) {
        const auto now = Clock::now();
        if (!connected_ && now >= nextHello_) {
            sendHello();
            nextHello_ = now + std::chrono::seconds(1);
        }
        if (connected_ && now >= nextHeartbeat_) {
            qbx::HeartbeatPayload hb{};
            hb.timestampNs = nowNs();
            sendPacket(qbx::PacketType::Heartbeat, &hb, sizeof(hb));
            nextHeartbeat_ = now + std::chrono::seconds(1);
        }

        // Reuse an uninitialized receive buffer; recvfrom writes only the received datagram.
        alignas(qbx::PacketHeader) std::array<uint8_t, 65507> buffer;
        for (int packets = 0; packets < 512; ++packets) {
            sockaddr_in peer{};
            socklen_t peerLength = sizeof(peer);
            const ssize_t received = recvfrom(socket_, buffer.data(), buffer.size(), 0,
                                              reinterpret_cast<sockaddr*>(&peer), &peerLength);
            if (received < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) break;
                LOGW("recvfrom failed errno=%d", errno);
                break;
            }
            // The configured PC may run the host and a separate video test sender (different source ports).
            if (peer.sin_addr.s_addr != host_.sin_addr.s_addr) continue;
            if (received < static_cast<ssize_t>(sizeof(qbx::PacketHeader))) continue;
            qbx::PacketHeader header{};
            std::memcpy(&header, buffer.data(), sizeof(header));
            if (header.magic != qbx::kMagic || header.version != qbx::kProtocolVersion ||
                header.payloadBytes != static_cast<uint32_t>(received - sizeof(qbx::PacketHeader))) continue;
            const uint8_t* payload = buffer.data() + sizeof(qbx::PacketHeader);
            switch (static_cast<qbx::PacketType>(header.type)) {
            case qbx::PacketType::HelloAck:
                if (header.payloadBytes == sizeof(qbx::HelloAckPayload)) {
                    qbx::HelloAckPayload ack{};
                    std::memcpy(&ack, payload, sizeof(ack));
                    connected_ = true;
                    LOGI("Windows host accepted connection (protocol=%u capabilities=0x%08x)",
                         ack.serverVersion, ack.acceptedCapabilities);
                }
                break;
            case qbx::PacketType::VideoConfig:
                handleVideoConfig(payload, header.payloadBytes, video);
                break;
            case qbx::PacketType::FrameChunk:
                handleFrameChunk(payload, header.payloadBytes, video);
                break;
            default:
                break;
            }
        }
        expireAssembly();
        video.pumpOutput();
    }

    void sendTracking(const qbx::TrackingPayload& payload) {
        if (connected_) sendPacket(qbx::PacketType::Tracking, &payload, sizeof(payload));
    }

    void sendController(const qbx::ControllerPayload& payload) {
        if (connected_) sendPacket(qbx::PacketType::Controller, &payload, sizeof(payload));
    }

private:
    bool sendPacket(qbx::PacketType type, const void* payload, uint32_t size) {
        if (socket_ < 0 || size + sizeof(qbx::PacketHeader) > qbx::kMaxUdpPayload) return false;
        std::array<uint8_t, qbx::kMaxUdpPayload> packet{};
        qbx::PacketHeader header{};
        header.magic = qbx::kMagic;
        header.version = qbx::kProtocolVersion;
        header.type = static_cast<uint16_t>(type);
        header.sequence = sequence_++;
        header.payloadBytes = size;
        std::memcpy(packet.data(), &header, sizeof(header));
        if (size && payload) std::memcpy(packet.data() + sizeof(header), payload, size);
        const size_t packetBytes = sizeof(header) + size;
        return sendto(socket_, packet.data(), packetBytes, 0,
                      reinterpret_cast<const sockaddr*>(&host_), sizeof(host_)) == static_cast<ssize_t>(packetBytes);
    }

    void sendHello() {
        qbx::HelloPayload hello{};
        hello.clientVersion = 2;
        hello.capabilities = kCapabilitiesTracking | kCapabilitiesControllers | kCapabilitiesH264 | kCapabilitiesHevc;
        std::snprintf(hello.deviceName, sizeof(hello.deviceName), "Meta Quest OpenXR");
        sendPacket(qbx::PacketType::Hello, &hello, sizeof(hello));
    }

    void handleVideoConfig(const uint8_t* payload, uint32_t bytes, VideoDecoder& video) {
        if (bytes != sizeof(qbx::VideoConfigPayload)) return;
        qbx::VideoConfigPayload packet{};
        std::memcpy(&packet, payload, sizeof(packet));
        QbxVideoConfig cfg{};
        cfg.width = packet.width;
        cfg.height = packet.height;
        cfg.framesPerSecond = packet.framesPerSecond;
        cfg.bitrateBitsPerSecond = packet.bitrateBitsPerSecond;
        cfg.codec = packet.codec;
        cfg.layout = packet.layout;
        if (packet.width > 4096 || packet.height > 4096 || packet.width == 0 || packet.height == 0) return;
        const auto now = Clock::now();
        const bool streamRestarted = lastFramePacketAt_ == Clock::time_point{} ||
            now - lastFramePacketAt_ > std::chrono::milliseconds(500);
        if (video.configure(cfg)) {
            assembly_.reset();
            if (streamRestarted) {
                // The test sender starts frame IDs from one after a process restart.
                // A quiet gap followed by VIDEO_CONFIG is our v1 stream-restart signal.
                hasStartedFrameId_ = false;
                lastStartedFrameId_ = 0;
            }
            LOGI("Received video config %ux%u @%u codec=%u layout=%u", cfg.width, cfg.height,
                 cfg.framesPerSecond, cfg.codec, cfg.layout);
        } else {
            LOGW("Video configuration rejected: %s", video.status());
        }
    }

    void handleFrameChunk(const uint8_t* payload, uint32_t bytes, VideoDecoder& video) {
        if (bytes < sizeof(qbx::FrameChunkPrefix) || !video.width()) return;
        qbx::FrameChunkPrefix prefix{};
        std::memcpy(&prefix, payload, sizeof(prefix));
        const size_t partBytes = bytes - sizeof(prefix);
        if (!prefix.chunkCount || prefix.chunkCount > 32768 || prefix.chunkIndex >= prefix.chunkCount ||
            prefix.totalFrameBytes == 0 || prefix.totalFrameBytes > maxFrameBytes_ ||
            partBytes == 0 || partBytes > prefix.totalFrameBytes) return;
        lastFramePacketAt_ = Clock::now();
        if (assembly_.chunks.empty() || assembly_.frameId != prefix.frameId) {
            if (hasStartedFrameId_ && static_cast<int32_t>(prefix.frameId - lastStartedFrameId_) <= 0) return;
            assembly_.reset();
            assembly_.frameId = prefix.frameId;
            assembly_.totalBytes = prefix.totalFrameBytes;
            assembly_.chunkCount = prefix.chunkCount;
            assembly_.codec = prefix.codec;
            assembly_.flags = prefix.flags;
            assembly_.timestampNs = prefix.timestampNs;
            assembly_.lastUpdate = Clock::now();
            assembly_.chunks.resize(prefix.chunkCount);
            assembly_.received.assign(prefix.chunkCount, 0);
            lastStartedFrameId_ = prefix.frameId;
            hasStartedFrameId_ = true;
        }
        if (assembly_.totalBytes != prefix.totalFrameBytes || assembly_.chunkCount != prefix.chunkCount ||
            assembly_.codec != prefix.codec) return;
        const size_t index = prefix.chunkIndex;
        if (!assembly_.received[index]) {
            assembly_.chunks[index].assign(payload + sizeof(prefix), payload + bytes);
            assembly_.received[index] = 1;
            assembly_.bytesReceived += partBytes;
            ++assembly_.chunksReceived;
        }
        assembly_.lastUpdate = Clock::now();
        if (assembly_.chunksReceived == assembly_.chunkCount) {
            if (assembly_.bytesReceived == assembly_.totalBytes) {
                std::vector<uint8_t> frame;
                frame.reserve(assembly_.totalBytes);
                for (const auto& chunk : assembly_.chunks) frame.insert(frame.end(), chunk.begin(), chunk.end());
                if (frame.size() == assembly_.totalBytes) {
                    // Use the sender's monotonic timestamp rather than frameId/fps. Frame IDs
                    // restart at one when the sender restarts; monotonic PTS avoids a backwards
                    // timestamp jump in MediaCodec during that transition.
                    const uint64_t ptsUs = assembly_.timestampNs / 1000ull;
                    video.pushFrame(frame.data(), frame.size(), ptsUs);
                }
            }
            assembly_.reset();
        }
    }

    void expireAssembly() {
        if (!assembly_.chunks.empty() && Clock::now() - assembly_.lastUpdate > std::chrono::milliseconds(incompleteFrameTimeoutMs_)) {
            assembly_.reset();
        }
    }

    ClientConfig config_{};
    int socket_ = -1;
    sockaddr_in host_{};
    uint32_t sequence_ = 1;
    bool connected_ = false;
    size_t maxFrameBytes_ = 16u * 1024u * 1024u;
    uint32_t incompleteFrameTimeoutMs_ = 120;
    uint32_t lastStartedFrameId_ = 0;
    bool hasStartedFrameId_ = false;
    Clock::time_point nextHello_{};
    Clock::time_point nextHeartbeat_{};
    Clock::time_point lastFramePacketAt_{};
    FrameAssembly assembly_{};
};

struct EyeSwapchain {
    XrSwapchain handle = XR_NULL_HANDLE;
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<XrSwapchainImageOpenGLESKHR> images;
};

struct HandActions {
    XrPath handPath = XR_NULL_PATH;
    XrAction pose = XR_NULL_HANDLE;
    XrAction trigger = XR_NULL_HANDLE;
    XrAction squeeze = XR_NULL_HANDLE;
    XrAction thumbstick = XR_NULL_HANDLE;
    XrAction primary = XR_NULL_HANDLE;
    XrAction secondary = XR_NULL_HANDLE;
    XrAction thumbstickClick = XR_NULL_HANDLE;
    XrAction menu = XR_NULL_HANDLE;
    XrAction squeezeClick = XR_NULL_HANDLE;
    XrAction triggerClick = XR_NULL_HANDLE;
    XrSpace poseSpace = XR_NULL_HANDLE;
};

class QuestRuntimeClient {
public:
    explicit QuestRuntimeClient(android_app* app) : app_(app), config_(loadConfig(app)) {}
    ~QuestRuntimeClient() { shutdown(); }

    bool initialize() {
        if (!network_.start(config_)) return false;
        if (!initializeEgl()) return false;
        if (!initializeOpenXr()) return false;
        if (!video_.initGl()) LOGW("Video renderer unavailable: %s", video_.status());
        if (!room_.initialize(app_ && app_->activity ? app_->activity->assetManager : nullptr, config_.room))
            LOGW("Room renderer initialization failed; check GLES shader logs");
        const int aspectMode = config_.aspectMode == "fit" ? 1 : config_.aspectMode == "crop" ? 2 : 0;
        video_.setImageControls(config_.brightness, config_.contrast, config_.saturation, config_.gamma,
                                aspectMode, config_.flipHorizontal, config_.flipVertical);
        video_.setSwapEyes(config_.swapEyes);
        return true;
    }

    void run() {
        bool exiting = false;
        while (!exiting && app_ && !app_->destroyRequested) {
            int events = 0;
            android_poll_source* source = nullptr;
            while (ALooper_pollOnce(sessionRunning_ ? 0 : 100, nullptr, &events,
                                   reinterpret_cast<void**>(&source)) >= 0) {
                if (source) source->process(app_, source);
                if (app_->destroyRequested) { exiting = true; break; }
            }
            if (exiting) break;
            pollXrEvents(exiting);
            network_.tick(video_);
            if (sessionRunning_) renderFrame();
        }
    }

private:
    bool initializeEgl() {
        display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (display_ == EGL_NO_DISPLAY || eglInitialize(display_, nullptr, nullptr) != EGL_TRUE) {
            LOGE("eglInitialize failed: 0x%x", eglGetError());
            return false;
        }
        if (eglBindAPI(EGL_OPENGL_ES_API) != EGL_TRUE) {
            LOGE("eglBindAPI(EGL_OPENGL_ES_API) failed: 0x%x", eglGetError());
            return false;
        }
        const EGLint configAttributes[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
            EGL_DEPTH_SIZE, 0, EGL_NONE
        };
        EGLint count = 0;
        if (eglChooseConfig(display_, configAttributes, &eglConfig_, 1, &count) != EGL_TRUE || count < 1) {
            LOGE("eglChooseConfig failed: 0x%x", eglGetError());
            return false;
        }
        const EGLint contextAttributes[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
        context_ = eglCreateContext(display_, eglConfig_, EGL_NO_CONTEXT, contextAttributes);
        if (context_ == EGL_NO_CONTEXT) {
            LOGE("eglCreateContext failed: 0x%x", eglGetError());
            return false;
        }
        if (!app_->window) {
            LOGE("NativeActivity window is null");
            return false;
        }
        surface_ = eglCreateWindowSurface(display_, eglConfig_, app_->window, nullptr);
        if (surface_ == EGL_NO_SURFACE || eglMakeCurrent(display_, surface_, surface_, context_) != EGL_TRUE) {
            LOGE("EGL surface/current failed: 0x%x", eglGetError());
            return false;
        }
        const char* glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
        LOGI("EGL initialized. OpenGL ES: %s", glVersion ? glVersion : "unknown");
        return true;
    }

    bool initializeOpenXr() {
        PFN_xrVoidFunction proc = nullptr;
        if (!xrOk(xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR", &proc), "xrGetInstanceProcAddr(xrInitializeLoaderKHR)")) return false;
        auto initializeLoader = reinterpret_cast<PFN_xrInitializeLoaderKHR>(proc);
        if (!initializeLoader) return false;
        XrLoaderInitInfoAndroidKHR loaderInfo{XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR};
        loaderInfo.applicationVM = app_->activity->vm;
        loaderInfo.applicationContext = app_->activity->clazz;
        if (!xrOk(initializeLoader(reinterpret_cast<const XrLoaderInitInfoBaseHeaderKHR*>(&loaderInfo)), "xrInitializeLoaderKHR")) return false;

        const char* extensions[] = {
            XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME,
            XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME
        };
        XrInstanceCreateInfoAndroidKHR androidInfo{XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR};
        androidInfo.applicationVM = app_->activity->vm;
        androidInfo.applicationActivity = app_->activity->clazz;
        XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
        createInfo.next = &androidInfo;
        std::snprintf(createInfo.applicationInfo.applicationName, XR_MAX_APPLICATION_NAME_SIZE, "QuestBridge XR");
        createInfo.applicationInfo.applicationVersion = 2;
        std::snprintf(createInfo.applicationInfo.engineName, XR_MAX_ENGINE_NAME_SIZE, "QuestBridge XR transport");
        createInfo.applicationInfo.engineVersion = 2;
        createInfo.applicationInfo.apiVersion = XR_MAKE_VERSION(1, 0, 0);
        createInfo.enabledExtensionCount = static_cast<uint32_t>(std::size(extensions));
        createInfo.enabledExtensionNames = extensions;
        if (!xrOk(xrCreateInstance(&createInfo, &instance_), "xrCreateInstance")) return false;

        XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
        systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
        if (!xrOk(xrGetSystem(instance_, &systemInfo, &systemId_), "xrGetSystem")) return false;
        PFN_xrGetOpenGLESGraphicsRequirementsKHR getGraphicsRequirements = nullptr;
        if (!xrOk(xrGetInstanceProcAddr(instance_, "xrGetOpenGLESGraphicsRequirementsKHR",
                                       reinterpret_cast<PFN_xrVoidFunction*>(&getGraphicsRequirements)),
                   "xrGetInstanceProcAddr(xrGetOpenGLESGraphicsRequirementsKHR)")) return false;
        XrGraphicsRequirementsOpenGLESKHR requirements{XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR};
        if (!xrOk(getGraphicsRequirements(instance_, systemId_, &requirements), "xrGetOpenGLESGraphicsRequirementsKHR")) return false;

        XrGraphicsBindingOpenGLESAndroidKHR graphicsBinding{XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR};
        graphicsBinding.display = display_;
        graphicsBinding.config = eglConfig_;
        graphicsBinding.context = context_;
        XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO};
        sessionInfo.next = &graphicsBinding;
        sessionInfo.systemId = systemId_;
        if (!xrOk(xrCreateSession(instance_, &sessionInfo, &session_), "xrCreateSession")) return false;

        XrReferenceSpaceType selectedReferenceSpace = XR_REFERENCE_SPACE_TYPE_LOCAL;
        if (config_.referenceSpace == "stage") {
            uint32_t supportedCount = 0;
            if (XR_SUCCEEDED(xrEnumerateReferenceSpaces(session_, 0, &supportedCount, nullptr)) && supportedCount > 0) {
                std::vector<XrReferenceSpaceType> supported(supportedCount);
                if (XR_SUCCEEDED(xrEnumerateReferenceSpaces(session_, supportedCount, &supportedCount, supported.data())) &&
                    std::find(supported.begin(), supported.end(), XR_REFERENCE_SPACE_TYPE_STAGE) != supported.end()) {
                    selectedReferenceSpace = XR_REFERENCE_SPACE_TYPE_STAGE;
                } else {
                    LOGW("STAGE reference space unavailable; falling back to LOCAL");
                }
            } else {
                LOGW("Could not enumerate reference spaces; falling back to LOCAL");
            }
        }
        XrReferenceSpaceCreateInfo spaceInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
        spaceInfo.referenceSpaceType = selectedReferenceSpace;
        spaceInfo.poseInReferenceSpace.orientation.w = 1.0f;
        if (!xrOk(xrCreateReferenceSpace(session_, &spaceInfo, &space_), "xrCreateReferenceSpace(LOCAL)")) return false;
        XrReferenceSpaceCreateInfo viewInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
        viewInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
        viewInfo.poseInReferenceSpace.orientation.w = 1.0f;
        if (!xrOk(xrCreateReferenceSpace(session_, &viewInfo, &viewSpace_), "xrCreateReferenceSpace(VIEW)")) return false;

        if (!createSwapchains()) return false;
        if (!createActions()) return false;
        LOGI("OpenXR initialized; waiting for session READY");
        return true;
    }

    bool createSwapchains() {
        uint32_t viewCount = 0;
        if (!xrOk(xrEnumerateViewConfigurationViews(instance_, systemId_, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                                                    0, &viewCount, nullptr), "xrEnumerateViewConfigurationViews(count)")) return false;
        if (viewCount != 2) {
            LOGE("Expected stereo views, got %u", viewCount);
            return false;
        }
        std::vector<XrViewConfigurationView> viewConfigs(viewCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
        if (!xrOk(xrEnumerateViewConfigurationViews(instance_, systemId_, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                                                    viewCount, &viewCount, viewConfigs.data()), "xrEnumerateViewConfigurationViews")) return false;
        views_.assign(viewCount, {XR_TYPE_VIEW});
        projectionViews_.assign(viewCount, {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW});

        uint32_t formatCount = 0;
        if (!xrOk(xrEnumerateSwapchainFormats(session_, 0, &formatCount, nullptr), "xrEnumerateSwapchainFormats(count)")) return false;
        std::vector<int64_t> formats(formatCount);
        if (!xrOk(xrEnumerateSwapchainFormats(session_, formatCount, &formatCount, formats.data()), "xrEnumerateSwapchainFormats")) return false;
        int64_t chosenFormat = formats.empty() ? 0 : formats[0];
        for (const int64_t format : formats) {
            if (format == GL_RGBA8) { chosenFormat = format; break; }
        }
        if (chosenFormat == 0) {
            LOGE("No supported OpenGL ES swapchain formats");
            return false;
        }

        swapchains_.resize(viewCount);
        for (uint32_t eye = 0; eye < viewCount; ++eye) {
            auto& sc = swapchains_[eye];
            sc.width = viewConfigs[eye].recommendedImageRectWidth;
            sc.height = viewConfigs[eye].recommendedImageRectHeight;
            XrSwapchainCreateInfo swapInfo{XR_TYPE_SWAPCHAIN_CREATE_INFO};
            swapInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
            swapInfo.format = chosenFormat;
            // The bring-up renderer attaches swapchain images directly to a single-sample FBO.
            swapInfo.sampleCount = 1;
            swapInfo.width = sc.width;
            swapInfo.height = sc.height;
            swapInfo.faceCount = 1;
            swapInfo.arraySize = 1;
            swapInfo.mipCount = 1;
            if (!xrOk(xrCreateSwapchain(session_, &swapInfo, &sc.handle), "xrCreateSwapchain")) return false;
            uint32_t imageCount = 0;
            if (!xrOk(xrEnumerateSwapchainImages(sc.handle, 0, &imageCount, nullptr), "xrEnumerateSwapchainImages(count)")) return false;
            sc.images.assign(imageCount, {XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR});
            if (!xrOk(xrEnumerateSwapchainImages(sc.handle, imageCount, &imageCount,
                        reinterpret_cast<XrSwapchainImageBaseHeader*>(sc.images.data())), "xrEnumerateSwapchainImages")) return false;
        }
        glGenFramebuffers(1, &framebuffer_);
        glGenRenderbuffers(1, &depthBuffer_);
        return true;
    }

    bool createActions() {
        XrActionSetCreateInfo setInfo{XR_TYPE_ACTION_SET_CREATE_INFO};
        std::snprintf(setInfo.actionSetName, XR_MAX_ACTION_SET_NAME_SIZE, "qbx_gameplay");
        std::snprintf(setInfo.localizedActionSetName, XR_MAX_LOCALIZED_ACTION_SET_NAME_SIZE, "QuestBridge input");
        setInfo.priority = 0;
        if (!xrOk(xrCreateActionSet(instance_, &setInfo, &actionSet_), "xrCreateActionSet")) return false;
        if (!createHandActions(0, "/user/hand/left", "left")) return false;
        if (!createHandActions(1, "/user/hand/right", "right")) return false;

        std::vector<XrActionSuggestedBinding> bindings;
        const std::string profileString = "/interaction_profiles/oculus/touch_controller";
        XrPath profile = XR_NULL_PATH;
        if (!xrOk(xrStringToPath(instance_, profileString.c_str(), &profile), "xrStringToPath(Touch profile)")) return false;
        auto bind = [&](XrAction action, const std::string& pathString) {
            XrPath path = XR_NULL_PATH;
            if (XR_SUCCEEDED(xrStringToPath(instance_, pathString.c_str(), &path))) bindings.push_back({action, path});
            else LOGW("Could not resolve input path %s", pathString.c_str());
        };
        bind(hands_[0].pose, std::string("/user/hand/left/input/") + config_.controllerPose + "/pose");
        bind(hands_[0].trigger, "/user/hand/left/input/trigger/value");
        bind(hands_[0].squeeze, "/user/hand/left/input/squeeze/value");
        bind(hands_[0].thumbstick, "/user/hand/left/input/thumbstick");
        bind(hands_[0].primary, "/user/hand/left/input/x/click");
        bind(hands_[0].secondary, "/user/hand/left/input/y/click");
        bind(hands_[0].thumbstickClick, "/user/hand/left/input/thumbstick/click");
        // Restore the left controller's three-line menu action. Some runtimes may reserve it,
        // but keep the application binding so runtimes that expose it can deliver the press.
        bind(hands_[0].menu, "/user/hand/left/input/menu/click");
        // OpenXR Touch profiles expose squeeze as a value, not a portable squeeze/click path.
        bind(hands_[1].pose, std::string("/user/hand/right/input/") + config_.controllerPose + "/pose");
        bind(hands_[1].trigger, "/user/hand/right/input/trigger/value");
        bind(hands_[1].squeeze, "/user/hand/right/input/squeeze/value");
        bind(hands_[1].thumbstick, "/user/hand/right/input/thumbstick");
        bind(hands_[1].primary, "/user/hand/right/input/a/click");
        bind(hands_[1].secondary, "/user/hand/right/input/b/click");
        bind(hands_[1].thumbstickClick, "/user/hand/right/input/thumbstick/click");
        XrInteractionProfileSuggestedBinding suggested{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
        suggested.interactionProfile = profile;
        suggested.countSuggestedBindings = static_cast<uint32_t>(bindings.size());
        suggested.suggestedBindings = bindings.data();
        if (!xrOk(xrSuggestInteractionProfileBindings(instance_, &suggested), "xrSuggestInteractionProfileBindings")) return false;

        XrSessionActionSetsAttachInfo attach{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
        attach.countActionSets = 1;
        attach.actionSets = &actionSet_;
        if (!xrOk(xrAttachSessionActionSets(session_, &attach), "xrAttachSessionActionSets")) return false;
        for (auto& hand : hands_) {
            XrActionSpaceCreateInfo spaceInfo{XR_TYPE_ACTION_SPACE_CREATE_INFO};
            spaceInfo.action = hand.pose;
            spaceInfo.subactionPath = hand.handPath;
            spaceInfo.poseInActionSpace.orientation.w = 1.0f;
            if (!xrOk(xrCreateActionSpace(session_, &spaceInfo, &hand.poseSpace), "xrCreateActionSpace(controller)")) return false;
        }
        return true;
    }

    bool createHandActions(int index, const char* pathString, const char* name) {
        HandActions& hand = hands_[index];
        if (!xrOk(xrStringToPath(instance_, pathString, &hand.handPath), "xrStringToPath(hand)")) return false;
        auto create = [&](XrActionType type, const char* suffix, XrAction* action) {
            XrActionCreateInfo info{XR_TYPE_ACTION_CREATE_INFO};
            std::snprintf(info.actionName, XR_MAX_ACTION_NAME_SIZE, "%s_%s", name, suffix);
            std::snprintf(info.localizedActionName, XR_MAX_LOCALIZED_ACTION_NAME_SIZE, "%s %s", name, suffix);
            info.actionType = type;
            info.countSubactionPaths = 1;
            info.subactionPaths = &hand.handPath;
            return xrOk(xrCreateAction(actionSet_, &info, action), info.actionName);
        };
        return create(XR_ACTION_TYPE_POSE_INPUT, "pose", &hand.pose) &&
               create(XR_ACTION_TYPE_FLOAT_INPUT, "trigger", &hand.trigger) &&
               create(XR_ACTION_TYPE_FLOAT_INPUT, "squeeze", &hand.squeeze) &&
               create(XR_ACTION_TYPE_VECTOR2F_INPUT, "thumbstick", &hand.thumbstick) &&
               create(XR_ACTION_TYPE_BOOLEAN_INPUT, "primary", &hand.primary) &&
               create(XR_ACTION_TYPE_BOOLEAN_INPUT, "secondary", &hand.secondary) &&
               create(XR_ACTION_TYPE_BOOLEAN_INPUT, "thumbstick_click", &hand.thumbstickClick) &&
               create(XR_ACTION_TYPE_BOOLEAN_INPUT, "menu", &hand.menu) &&
               create(XR_ACTION_TYPE_BOOLEAN_INPUT, "squeeze_click", &hand.squeezeClick) &&
               true; // Trigger click is derived from the analog trigger value; Quest Touch does not expose a portable trigger/click binding.
    }

    bool boolAction(XrAction action, XrPath handPath) {
        XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO};
        getInfo.action = action;
        getInfo.subactionPath = handPath;
        XrActionStateBoolean state{XR_TYPE_ACTION_STATE_BOOLEAN};
        if (XR_SUCCEEDED(xrGetActionStateBoolean(session_, &getInfo, &state))) return state.isActive && state.currentState;
        return false;
    }

    float floatAction(XrAction action, XrPath handPath) {
        XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO};
        getInfo.action = action;
        getInfo.subactionPath = handPath;
        XrActionStateFloat state{XR_TYPE_ACTION_STATE_FLOAT};
        if (XR_SUCCEEDED(xrGetActionStateFloat(session_, &getInfo, &state))) return state.isActive ? state.currentState : 0.0f;
        return 0.0f;
    }

    XrVector2f vector2Action(XrAction action, XrPath handPath) {
        XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO};
        getInfo.action = action;
        getInfo.subactionPath = handPath;
        XrActionStateVector2f state{XR_TYPE_ACTION_STATE_VECTOR2F};
        if (XR_SUCCEEDED(xrGetActionStateVector2f(session_, &getInfo, &state)) && state.isActive) return state.currentState;
        return {0.0f, 0.0f};
    }

    void pollXrEvents(bool& exiting) {
        if (!instance_) return;
        XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};
        while (xrPollEvent(instance_, &event) == XR_SUCCESS) {
            switch (event.type) {
            case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
                const auto* changed = reinterpret_cast<const XrEventDataSessionStateChanged*>(&event);
                sessionState_ = changed->state;
                LOGI("OpenXR session state changed: %d", static_cast<int>(sessionState_));
                if (sessionState_ == XR_SESSION_STATE_READY && !sessionRunning_) {
                    XrSessionBeginInfo beginInfo{XR_TYPE_SESSION_BEGIN_INFO};
                    beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
                    if (xrOk(xrBeginSession(session_, &beginInfo), "xrBeginSession")) sessionRunning_ = true;
                } else if (sessionState_ == XR_SESSION_STATE_STOPPING && sessionRunning_) {
                    xrEndSession(session_);
                    sessionRunning_ = false;
                } else if (sessionState_ == XR_SESSION_STATE_EXITING || sessionState_ == XR_SESSION_STATE_LOSS_PENDING) {
                    exiting = true;
                }
                break;
            }
            case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
                exiting = true;
                break;
            default:
                break;
            }
            event = {XR_TYPE_EVENT_DATA_BUFFER};
        }
    }

    bool locatePose(XrSpace space, XrTime time, XrPosef& pose, uint32_t& flags) {
        pose = {};
        pose.orientation.w = 1.0f;
        flags = 0;
        XrSpaceLocation location{XR_TYPE_SPACE_LOCATION};
        if (!XR_SUCCEEDED(xrLocateSpace(space, space_, time, &location))) {
            flags = 0;
            return false;
        }
        pose = location.pose;
        flags = 0;
        if (location.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) {
            flags |= 1;
        } else {
            pose.position = {0.0f, 0.0f, 0.0f};
        }
        if (location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) {
            flags |= 2;
        } else {
            pose.orientation = {0.0f, 0.0f, 0.0f, 1.0f};
        }
        return flags != 0;
    }

    static float applyDeadzone(float value, float deadzone) {
        const float magnitude = std::fabs(value);
        if (magnitude <= deadzone) return 0.0f;
        const float scaled = (magnitude - deadzone) / (1.0f - deadzone);
        return std::copysign(std::clamp(scaled, 0.0f, 1.0f), value);
    }

    static float applyTriggerCurve(float value, float deadzone, float curve) {
        const float normalized = std::clamp((value - deadzone) / std::max(1.0f - deadzone, 0.001f), 0.0f, 1.0f);
        return std::pow(normalized, curve);
    }

    void sendTracking(XrTime predictedTime) {
        const auto now = Clock::now();
        const auto minimumInterval = std::chrono::microseconds(1000000 / std::max<uint16_t>(10, config_.trackingHz));
        if (lastTrackingSend_ != Clock::time_point{} && now - lastTrackingSend_ < minimumInterval) return;
        const float dt = lastTrackingSend_ == Clock::time_point{} ? 0.0f :
            std::chrono::duration<float>(now - lastTrackingSend_).count();
        lastTrackingSend_ = now;

        qbx::TrackingPayload tracking{};
        tracking.timestampNs = nowNs();
        tracking.headOrientation[3] = 1.0f;
        XrPosef headPose{};
        headPose.orientation.w = 1.0f;
        uint32_t flags = 0;
        locatePose(viewSpace_, predictedTime, headPose, flags);
        if (config_.trackingEnabled) {
            tracking.headPosition[0] = headPose.position.x * config_.positionScale + config_.offsetX;
            tracking.headPosition[1] = headPose.position.y * config_.positionScale + config_.offsetY;
            tracking.headPosition[2] = headPose.position.z * config_.positionScale + config_.offsetZ;
            tracking.headOrientation[0] = headPose.orientation.x;
            tracking.headOrientation[1] = headPose.orientation.y;
            tracking.headOrientation[2] = headPose.orientation.z;
            tracking.headOrientation[3] = headPose.orientation.w;
            tracking.trackingFlags = flags;
            network_.sendTracking(tracking);
        }

        // Always sync actions: left stick navigates the room and right stick turns it even
        // when controller telemetry forwarding is disabled.
        XrActiveActionSet active{actionSet_, XR_NULL_PATH};
        XrActionsSyncInfo syncInfo{XR_TYPE_ACTIONS_SYNC_INFO};
        syncInfo.countActiveActionSets = 1;
        syncInfo.activeActionSets = &active;
        if (XR_FAILED(xrSyncActions(session_, &syncInfo))) return;
        const XrVector2f move = vector2Action(hands_[0].thumbstick, hands_[0].handPath);
        const XrVector2f turn = vector2Action(hands_[1].thumbstick, hands_[1].handPath);
        // Quest may reserve the physical system-menu button; left Y is a deliberate in-app fallback.
        const bool menuPressed = boolAction(hands_[0].menu, hands_[0].handPath) ||
                                 boolAction(hands_[0].secondary, hands_[0].handPath);
        if (menuPressed && !previousMenuPressed_) room_.toggleMenu();
        previousMenuPressed_ = menuPressed;
        const bool menuSelectPressed = boolAction(hands_[0].primary, hands_[0].handPath);
        const bool menuSelectEdge = menuSelectPressed && !previousMenuSelectPressed_;
        previousMenuSelectPressed_ = menuSelectPressed;
        // OpenXR thumbstick +Y is forward. The room renderer treats positive Y as forward (-Z),
        // so preserve that sign unless the user explicitly enables inversion.
        float moveX = applyDeadzone(std::clamp(config_.invertThumbstickX ? -move.x : move.x, -1.0f, 1.0f), config_.thumbstickDeadzone);
        float moveY = applyDeadzone(std::clamp(config_.invertThumbstickY ? -move.y : move.y, -1.0f, 1.0f), config_.thumbstickDeadzone);
        const float turnX = applyDeadzone(std::clamp(turn.x, -1.0f, 1.0f), config_.thumbstickDeadzone);
        if (room_.menuVisible()) {
            room_.handleMenuInput(move.y, menuSelectEdge);
            moveX = 0.0f;
            moveY = 0.0f;
        }
        // Walking can be disabled independently; turning must remain available in either mode.
        if (!config_.roomMovementEnabled) { moveX = 0.0f; moveY = 0.0f; }
        room_.updateLocomotion(moveX, moveY, turnX, dt, headPose);

        if (!config_.controllersEnabled) return;
        for (int handIndex = 0; handIndex < 2; ++handIndex) {
            const HandActions& hand = hands_[handIndex];
            qbx::ControllerPayload out{};
            out.timestampNs = tracking.timestampNs;
            out.hand = static_cast<uint8_t>(config_.swapHands ? 1 - handIndex : handIndex);
            XrPosef controllerPose{};
            controllerPose.orientation.w = 1.0f;
            uint32_t controllerFlags = 0;
            locatePose(hand.poseSpace, predictedTime, controllerPose, controllerFlags);
            out.trackingFlags = static_cast<uint8_t>(controllerFlags);
            out.position[0] = controllerPose.position.x * config_.positionScale + config_.offsetX;
            out.position[1] = controllerPose.position.y * config_.positionScale + config_.offsetY;
            out.position[2] = controllerPose.position.z * config_.positionScale + config_.offsetZ;
            out.orientation[0] = controllerPose.orientation.x;
            out.orientation[1] = controllerPose.orientation.y;
            out.orientation[2] = controllerPose.orientation.z;
            out.orientation[3] = controllerPose.orientation.w;
            out.trigger = applyTriggerCurve(std::clamp(floatAction(hand.trigger, hand.handPath), 0.0f, 1.0f),
                                            config_.triggerDeadzone, config_.triggerCurve);
            out.grip = std::clamp(floatAction(hand.squeeze, hand.handPath), 0.0f, 1.0f);
            const XrVector2f stick = vector2Action(hand.thumbstick, hand.handPath);
            const float stickX = config_.invertThumbstickX ? -stick.x : stick.x;
            const float stickY = config_.invertThumbstickY ? -stick.y : stick.y;
            out.thumbstick[0] = applyDeadzone(std::clamp(stickX, -1.0f, 1.0f), config_.thumbstickDeadzone);
            out.thumbstick[1] = applyDeadzone(std::clamp(stickY, -1.0f, 1.0f), config_.thumbstickDeadzone);
            bool primaryPressed = boolAction(hand.primary, hand.handPath);
            bool secondaryPressed = boolAction(hand.secondary, hand.handPath);
            if (config_.swapFaceButtons) std::swap(primaryPressed, secondaryPressed);
            if (primaryPressed) out.buttons |= 1u << 0;
            if (secondaryPressed) out.buttons |= 1u << 1;
            if (boolAction(hand.thumbstickClick, hand.handPath)) out.buttons |= 1u << 2;
            if (handIndex == 0 && boolAction(hand.menu, hand.handPath)) out.buttons |= 1u << 3;
            if (boolAction(hand.squeezeClick, hand.handPath)) out.buttons |= 1u << 4;
            if (floatAction(hand.trigger, hand.handPath) >= 0.75f) out.buttons |= 1u << 5;
            network_.sendController(out);
        }
    }

    void renderFrame() {
        XrFrameWaitInfo waitInfo{XR_TYPE_FRAME_WAIT_INFO};
        XrFrameState frameState{XR_TYPE_FRAME_STATE};
        if (!xrOk(xrWaitFrame(session_, &waitInfo, &frameState), "xrWaitFrame")) return;
        XrFrameBeginInfo beginInfo{XR_TYPE_FRAME_BEGIN_INFO};
        if (!xrOk(xrBeginFrame(session_, &beginInfo), "xrBeginFrame")) return;

        bool layerReady = false;
        if (frameState.shouldRender) {
            XrViewState viewState{XR_TYPE_VIEW_STATE};
            XrViewLocateInfo locateInfo{XR_TYPE_VIEW_LOCATE_INFO};
            locateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
            locateInfo.displayTime = frameState.predictedDisplayTime;
            locateInfo.space = space_;
            uint32_t viewCount = 0;
            const XrResult located = xrLocateViews(session_, &locateInfo, &viewState,
                                                   static_cast<uint32_t>(views_.size()), &viewCount, views_.data());
            if (XR_SUCCEEDED(located) && viewCount == views_.size()) {
                sendTracking(frameState.predictedDisplayTime);
                video_.pumpOutput();
                bool allEyesRendered = true;
                for (uint32_t eye = 0; eye < viewCount; ++eye) {
                    auto& sc = swapchains_[eye];
                    uint32_t imageIndex = 0;
                    XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
                    if (XR_FAILED(xrAcquireSwapchainImage(sc.handle, &acquireInfo, &imageIndex))) {
                        allEyesRendered = false;
                        break;
                    }
                    XrSwapchainImageWaitInfo waitSwapInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
                    waitSwapInfo.timeout = XR_INFINITE_DURATION;
                    if (XR_FAILED(xrWaitSwapchainImage(sc.handle, &waitSwapInfo))) {
                        XrSwapchainImageReleaseInfo rel{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
                        xrReleaseSwapchainImage(sc.handle, &rel);
                        allEyesRendered = false;
                        break;
                    }
                    const GLuint desktopTexture = video_.renderEyeTexture(static_cast<int>(eye), 1024, 576);
                    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                           sc.images[imageIndex].image, 0);
                    glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer_);
                    if (depthWidth_ != static_cast<int>(sc.width) || depthHeight_ != static_cast<int>(sc.height)) {
                        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,
                                              static_cast<GLsizei>(sc.width), static_cast<GLsizei>(sc.height));
                        depthWidth_ = static_cast<int>(sc.width); depthHeight_ = static_cast<int>(sc.height);
                    }
                    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer_);
                    glViewport(0, 0, static_cast<GLsizei>(sc.width), static_cast<GLsizei>(sc.height));
                    const float shade = eye == 0 ? 0.035f : 0.055f;
                    glClearColor(shade, shade, shade + 0.025f, 1.0f);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                    room_.renderEye(static_cast<int>(eye), static_cast<int>(sc.width), static_cast<int>(sc.height),
                                    views_[eye], desktopTexture, video_.hasFrame());
                    glBindFramebuffer(GL_FRAMEBUFFER, 0);
                    XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
                    if (XR_FAILED(xrReleaseSwapchainImage(sc.handle, &releaseInfo))) {
                        allEyesRendered = false;
                        break;
                    }

                    projectionViews_[eye] = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
                    projectionViews_[eye].pose = views_[eye].pose;
                    projectionViews_[eye].fov = views_[eye].fov;
                    projectionViews_[eye].subImage.swapchain = sc.handle;
                    projectionViews_[eye].subImage.imageRect.offset = {0, 0};
                    projectionViews_[eye].subImage.imageRect.extent = {static_cast<int32_t>(sc.width), static_cast<int32_t>(sc.height)};
                    projectionViews_[eye].subImage.imageArrayIndex = 0;
                }
                layerReady = allEyesRendered;
            } else {
                LOGW("xrLocateViews failed or returned an unexpected number of views");
                sendTracking(frameState.predictedDisplayTime);
            }
        } else {
            // Keep tracking/input flowing while the runtime asks the app not to render.
            sendTracking(frameState.predictedDisplayTime);
        }

        XrCompositionLayerProjection layer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
        layer.space = space_;
        layer.viewCount = static_cast<uint32_t>(projectionViews_.size());
        layer.views = projectionViews_.data();
        const XrCompositionLayerBaseHeader* layers[] = {reinterpret_cast<const XrCompositionLayerBaseHeader*>(&layer)};
        XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
        endInfo.displayTime = frameState.predictedDisplayTime;
        endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
        if (layerReady) {
            endInfo.layerCount = 1;
            endInfo.layers = layers;
        }
        xrEndFrame(session_, &endInfo);
    }

    void shutdown() {
        if (shutdown_) return;
        shutdown_ = true;
        if (session_ && sessionRunning_) {
            xrEndSession(session_);
            sessionRunning_ = false;
        }
        for (auto& hand : hands_) {
            if (hand.poseSpace) xrDestroySpace(hand.poseSpace);
            hand.poseSpace = XR_NULL_HANDLE;
        }
        for (auto& sc : swapchains_) {
            if (sc.handle) xrDestroySwapchain(sc.handle);
            sc.handle = XR_NULL_HANDLE;
        }
        room_.release();
        video_.releaseGl();
        if (framebuffer_) glDeleteFramebuffers(1, &framebuffer_);
        if (depthBuffer_) glDeleteRenderbuffers(1, &depthBuffer_);
        framebuffer_ = 0; depthBuffer_ = 0;
        if (space_) xrDestroySpace(space_);
        if (viewSpace_) xrDestroySpace(viewSpace_);
        if (session_) xrDestroySession(session_);
        if (actionSet_) xrDestroyActionSet(actionSet_);
        if (instance_) xrDestroyInstance(instance_);
        instance_ = XR_NULL_HANDLE;
        if (display_ != EGL_NO_DISPLAY) {
            eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            if (surface_ != EGL_NO_SURFACE) eglDestroySurface(display_, surface_);
            if (context_ != EGL_NO_CONTEXT) eglDestroyContext(display_, context_);
            eglTerminate(display_);
        }
        surface_ = EGL_NO_SURFACE;
        context_ = EGL_NO_CONTEXT;
        display_ = EGL_NO_DISPLAY;
    }

    android_app* app_ = nullptr;
    ClientConfig config_{};
    NetworkClient network_{};
    VideoDecoder video_{};
    RoomRenderer room_{};
    GLuint depthBuffer_ = 0;
    int depthWidth_ = 0;
    int depthHeight_ = 0;
    EGLDisplay display_ = EGL_NO_DISPLAY;
    EGLConfig eglConfig_ = nullptr;
    EGLContext context_ = EGL_NO_CONTEXT;
    EGLSurface surface_ = EGL_NO_SURFACE;
    XrInstance instance_ = XR_NULL_HANDLE;
    XrSystemId systemId_ = XR_NULL_SYSTEM_ID;
    XrSession session_ = XR_NULL_HANDLE;
    XrSpace space_ = XR_NULL_HANDLE;
    XrSpace viewSpace_ = XR_NULL_HANDLE;
    XrSessionState sessionState_ = XR_SESSION_STATE_UNKNOWN;
    bool sessionRunning_ = false;
    bool shutdown_ = false;
    XrActionSet actionSet_ = XR_NULL_HANDLE;
    std::array<HandActions, 2> hands_{};
    std::vector<EyeSwapchain> swapchains_;
    std::vector<XrView> views_;
    std::vector<XrCompositionLayerProjectionView> projectionViews_;
    GLuint framebuffer_ = 0;
    Clock::time_point lastTrackingSend_{};
    bool previousMenuPressed_ = false;
    bool previousMenuSelectPressed_ = false;
};

} // namespace

void android_main(android_app* app) {
    app_dummy();
    if (!app) return;
    // NativeActivity creates the window before the immersive activity is launched, but wait defensively.
    while (!app->window && !app->destroyRequested) {
        int events = 0;
        android_poll_source* source = nullptr;
        ALooper_pollOnce(100, nullptr, &events, reinterpret_cast<void**>(&source));
        if (source) source->process(app, source);
    }
    if (app->destroyRequested) return;
    QuestRuntimeClient client(app);
    if (!client.initialize()) {
        LOGE("QuestBridge XR initialization failed; check logcat and config.ini");
        return;
    }
    client.run();
    LOGI("QuestBridge XR exited cleanly");
}
