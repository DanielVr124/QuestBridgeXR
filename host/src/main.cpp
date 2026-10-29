#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>
#include <stdexcept>
#include <thread>

#include "qbx/protocol.hpp"
#include "fake_vr_bridge.hpp"

#pragma comment(lib, "Ws2_32.lib")

namespace {
std::atomic<bool> g_running{true};
SOCKET g_socket = INVALID_SOCKET;
SOCKET g_telemetrySocket = INVALID_SOCKET;
sockaddr_in g_telemetryPeer{};
sockaddr_in g_clientPeer{};
bool g_clientKnown = false;
FakeVrBridge g_fakeVr;
std::uint64_t g_telemetryForwarded = 0;
std::uint64_t g_telemetryForwardErrors = 0;

BOOL WINAPI consoleHandler(DWORD event) {
    if (event == CTRL_C_EVENT || event == CTRL_CLOSE_EVENT || event == CTRL_BREAK_EVENT) {
        g_running.store(false);
        if (g_socket != INVALID_SOCKET) shutdown(g_socket, SD_BOTH);
        return TRUE;
    }
    return FALSE;
}

const char* typeName(qbx::PacketType type) {
    using qbx::PacketType;
    switch (type) {
    case PacketType::Hello: return "HELLO";
    case PacketType::HelloAck: return "HELLO_ACK";
    case PacketType::Tracking: return "TRACKING";
    case PacketType::Controller: return "CONTROLLER";
    case PacketType::Heartbeat: return "HEARTBEAT";
    case PacketType::FrameChunk: return "FRAME_CHUNK";
    case PacketType::Goodbye: return "GOODBYE";
    case PacketType::VideoConfig: return "VIDEO_CONFIG";
    default: return "UNKNOWN";
    }
}

bool sendPacket(SOCKET sock, const sockaddr_in& peer, qbx::PacketType type,
                std::uint32_t sequence, const void* payload, std::uint32_t payloadBytes) {
    // TrackingPayload is the largest payload sent by this host-side helper.
    std::array<std::uint8_t, sizeof(qbx::PacketHeader) + sizeof(qbx::TrackingPayload)> buffer{};
    if (sizeof(qbx::PacketHeader) + payloadBytes > buffer.size()) return false;

    qbx::PacketHeader header{};
    header.magic = qbx::kMagic;
    header.version = qbx::kProtocolVersion;
    header.type = static_cast<std::uint16_t>(type);
    header.sequence = sequence;
    header.payloadBytes = payloadBytes;
    std::memcpy(buffer.data(), &header, sizeof(header));
    if (payloadBytes != 0 && payload != nullptr) {
        std::memcpy(buffer.data() + sizeof(header), payload, payloadBytes);
    }
    const int bytes = static_cast<int>(sizeof(header) + payloadBytes);
    return sendto(sock, reinterpret_cast<const char*>(buffer.data()), bytes, 0,
                  reinterpret_cast<const sockaddr*>(&peer), sizeof(peer)) == bytes;
}

bool samePeer(const sockaddr_in& a, const sockaddr_in& b) {
    return a.sin_family == b.sin_family && a.sin_addr.s_addr == b.sin_addr.s_addr && a.sin_port == b.sin_port;
}

bool validHeader(const qbx::PacketHeader& header, int receivedBytes) {
    return receivedBytes >= static_cast<int>(sizeof(header)) &&
           header.magic == qbx::kMagic &&
           header.version == qbx::kProtocolVersion &&
           header.payloadBytes == static_cast<std::uint32_t>(receivedBytes - static_cast<int>(sizeof(header)));
}
}

int main(int argc, char** argv) {
    unsigned long port = qbx::kDefaultPort;
    unsigned long telemetryPort = 9945; // 0 disables the localhost pose/input feed.
    try {
        if (argc >= 2) {
            const std::string arg = argv[1];
            std::size_t used = 0;
            port = std::stoul(arg, &used);
            if (used != arg.size() || port == 0 || port > 65535) throw std::invalid_argument("host port");
        }
        if (argc >= 3) {
            const std::string arg = argv[2];
            std::size_t used = 0;
            telemetryPort = std::stoul(arg, &used);
            if (used != arg.size() || telemetryPort > 65535) throw std::invalid_argument("telemetry port");
        }
        if (telemetryPort != 0 && telemetryPort == port) throw std::invalid_argument("ports must differ");
        if (argc > 3) throw std::invalid_argument("too many arguments");
    } catch (...) {
        std::cerr << "Usage: QuestBridgeHost.exe [host_port 1-65535] [telemetry_port 0-65535]\n"
                  << "Set telemetry_port to 0 to disable the localhost tracking/controller feed.\n";
        return 2;
    }

    WSADATA wsa{};
    const int startup = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (startup != 0) {
        std::cerr << "WSAStartup failed: " << startup << "\n";
        return 1;
    }

    g_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (g_socket == INVALID_SOCKET) {
        std::cerr << "socket failed: " << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }
    DWORD timeoutMs = 500;
    setsockopt(g_socket, SOL_SOCKET, SO_RCVTIMEO,
               reinterpret_cast<const char*>(&timeoutMs), sizeof(timeoutMs));

    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    local.sin_port = htons(static_cast<u_short>(port));
    if (bind(g_socket, reinterpret_cast<const sockaddr*>(&local), sizeof(local)) == SOCKET_ERROR) {
        std::cerr << "bind failed: " << WSAGetLastError() << " (port " << port << ")\n";
        closesocket(g_socket);
        WSACleanup();
        return 1;
    }

    if (telemetryPort != 0) {
        g_telemetrySocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (g_telemetrySocket == INVALID_SOCKET) {
            std::cerr << "Could not create localhost telemetry socket (" << WSAGetLastError()
                      << "); telemetry forwarding disabled.\n";
        } else {
            g_telemetryPeer.sin_family = AF_INET;
            g_telemetryPeer.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            g_telemetryPeer.sin_port = htons(static_cast<u_short>(telemetryPort));
        }
    }

    g_fakeVr.load("config/fake_vr.ini");
    SetConsoleCtrlHandler(consoleHandler, TRUE);
    std::cout << "QuestBridge XR host + experimental head-look bridge\n"
              << "Listening on UDP 0.0.0.0:" << port << "\n";
    if (g_telemetrySocket != INVALID_SOCKET) {
        std::cout << "Forwarding validated client TRACKING/CONTROLLER packets to 127.0.0.1:"
                  << telemetryPort << "\n";
    } else {
        std::cout << "Local telemetry feed disabled.\n";
    }
    std::cout << "Waiting for HELLO / tracking / controller packets. Ctrl+C to stop.\n"
              << "Note: FFmpeg handles video encoding; this host is diagnostic/input-bridge code, not a Windows OpenXR runtime/compositor.\n\n";

    alignas(qbx::PacketHeader) std::array<std::uint8_t, 65507> buffer{};
    auto lastTrackingPrint = std::chrono::steady_clock::now() - std::chrono::seconds(2);
    std::uint32_t lastButtons[2] = {0xffffffffu, 0xffffffffu};
    std::uint64_t receivedCount = 0;
    std::uint64_t videoChunksSinceReport = 0;
    auto lastVideoReport = std::chrono::steady_clock::now();

    while (g_running.load()) {
        g_fakeVr.pollHotkeys();
        sockaddr_in peer{};
        int peerLen = sizeof(peer);
        const int bytes = recvfrom(g_socket, reinterpret_cast<char*>(buffer.data()),
                                   static_cast<int>(buffer.size()), 0,
                                   reinterpret_cast<sockaddr*>(&peer), &peerLen);
        if (bytes == SOCKET_ERROR) {
            const int err = WSAGetLastError();
            if (err == WSAETIMEDOUT || err == WSAEWOULDBLOCK) continue;
            if (!g_running.load() || err == WSAENOTSOCK || err == WSAESHUTDOWN) break;
            std::cerr << "recvfrom failed: " << err << "\n";
            continue;
        }
        if (bytes < static_cast<int>(sizeof(qbx::PacketHeader))) {
            std::cerr << "Dropped short datagram (" << bytes << " bytes)\n";
            continue;
        }

        qbx::PacketHeader header{};
        std::memcpy(&header, buffer.data(), sizeof(header));
        if (!validHeader(header, bytes)) {
            std::cerr << "Dropped invalid packet header\n";
            continue;
        }
        const auto type = static_cast<qbx::PacketType>(header.type);
        const auto* payload = buffer.data() + sizeof(header);
        ++receivedCount;

        char address[INET_ADDRSTRLEN]{};
        inet_ntop(AF_INET, &peer.sin_addr, address, sizeof(address));
        const unsigned peerPort = ntohs(peer.sin_port);

        switch (type) {
        case qbx::PacketType::Hello: {
            if (header.payloadBytes != sizeof(qbx::HelloPayload)) {
                std::cerr << "Dropped HELLO with wrong payload size\n";
                break;
            }
            qbx::HelloPayload hello{};
            std::memcpy(&hello, payload, sizeof(hello));
            hello.deviceName[sizeof(hello.deviceName) - 1] = '\0';
            g_clientPeer = peer;
            g_clientKnown = true;
            std::cout << "Client HELLO from " << address << ':' << peerPort
                      << " | device='" << hello.deviceName << "'"
                      << " | clientVersion=" << hello.clientVersion << "\n";
            qbx::HelloAckPayload ack{};
            ack.serverVersion = qbx::kProtocolVersion;
            ack.acceptedCapabilities = hello.capabilities;
            ack.streamPort = static_cast<std::uint16_t>(port);
            if (!sendPacket(g_socket, peer, qbx::PacketType::HelloAck, header.sequence, &ack, sizeof(ack))) {
                std::cerr << "Failed to send HELLO_ACK\n";
            }
            break;
        }
        case qbx::PacketType::Tracking: {
            if (header.payloadBytes != sizeof(qbx::TrackingPayload)) {
                std::cerr << "Dropped TRACKING with wrong payload size\n";
                break;
            }
            // Only accept camera-control input from the connected Quest peer established by HELLO.
            // Fake VR uses regular SendInput mouse deltas; it never injects into or patches a game.
            if (g_clientKnown && samePeer(peer, g_clientPeer)) {
                qbx::TrackingPayload tracking{};
                std::memcpy(&tracking, payload, sizeof(tracking));
                g_fakeVr.onTracking(tracking);
                if (g_telemetrySocket != INVALID_SOCKET) {
                    if (sendPacket(g_telemetrySocket, g_telemetryPeer, qbx::PacketType::Tracking,
                                   header.sequence, payload, header.payloadBytes)) ++g_telemetryForwarded;
                    else ++g_telemetryForwardErrors;
                }
            }
            const auto now = std::chrono::steady_clock::now();
            if (now - lastTrackingPrint >= std::chrono::seconds(1)) {
                qbx::TrackingPayload p{};
                std::memcpy(&p, payload, sizeof(p));
                std::cout << std::fixed << std::setprecision(3)
                          << "HMD " << address << " pos[m]=(" << p.headPosition[0] << ", "
                          << p.headPosition[1] << ", " << p.headPosition[2] << ")"
                          << " quat=(" << p.headOrientation[0] << ", " << p.headOrientation[1]
                          << ", " << p.headOrientation[2] << ", " << p.headOrientation[3] << ")"
                          << " valid=0x" << std::hex << p.trackingFlags << std::dec << "\n";
                lastTrackingPrint = now;
            }
            break;
        }
        case qbx::PacketType::Controller: {
            if (header.payloadBytes != sizeof(qbx::ControllerPayload)) {
                std::cerr << "Dropped CONTROLLER with wrong payload size\n";
                break;
            }
            qbx::ControllerPayload p{};
            std::memcpy(&p, payload, sizeof(p));
            if (p.hand > 1) {
                std::cerr << "Dropped CONTROLLER with invalid hand index\n";
                break;
            }
            if (g_telemetrySocket != INVALID_SOCKET && g_clientKnown && samePeer(peer, g_clientPeer)) {
                if (sendPacket(g_telemetrySocket, g_telemetryPeer, qbx::PacketType::Controller,
                               header.sequence, payload, header.payloadBytes)) ++g_telemetryForwarded;
                else ++g_telemetryForwardErrors;
            }
            if (lastButtons[p.hand] != p.buttons) {
                std::cout << (p.hand == 0 ? "Left" : "Right") << " controller buttons=0x"
                          << std::hex << p.buttons << std::dec
                          << " trigger=" << std::fixed << std::setprecision(2) << p.trigger
                          << " grip=" << p.grip << " stick=(" << p.thumbstick[0]
                          << ", " << p.thumbstick[1] << ")\n";
                lastButtons[p.hand] = p.buttons;
            }
            break;
        }
        case qbx::PacketType::Heartbeat:
            break;
        case qbx::PacketType::Goodbye:
            std::cout << "Client GOODBYE from " << address << ':' << peerPort << "\n";
            break;
        case qbx::PacketType::VideoConfig: {
            if (header.payloadBytes != sizeof(qbx::VideoConfigPayload)) {
                std::cerr << "Dropped VIDEO_CONFIG with wrong payload size\n";
                break;
            }
            qbx::VideoConfigPayload cfg{};
            std::memcpy(&cfg, payload, sizeof(cfg));
            std::cout << "VIDEO_CONFIG " << cfg.width << "x" << cfg.height
                      << " @ " << cfg.framesPerSecond << " fps, codec=" << cfg.codec
                      << ", layout=" << cfg.layout << "\n";
            break;
        }
        case qbx::PacketType::FrameChunk:
            // High-volume video chunks are intentionally summarized once per second below.
            ++videoChunksSinceReport;
            break;
        default:
            std::cerr << "Received unsupported packet type " << header.type
                      << " (" << typeName(type) << ")\n";
            break;
        }

        const auto reportNow = std::chrono::steady_clock::now();
        if (reportNow - lastVideoReport >= std::chrono::seconds(1)) {
            if (videoChunksSinceReport > 0) {
                std::cout << "Video ingress: " << videoChunksSinceReport
                          << " UDP frame chunks/s (host is diagnostic-only; Quest receives video directly).\n";
            }
            videoChunksSinceReport = 0;
            lastVideoReport = reportNow;
        }
    }

    std::cout << "Stopping; received " << receivedCount << " valid packets; forwarded telemetry="
              << g_telemetryForwarded << " relay errors=" << g_telemetryForwardErrors << ".\n";
    if (g_telemetrySocket != INVALID_SOCKET) closesocket(g_telemetrySocket);
    g_telemetrySocket = INVALID_SOCKET;
    closesocket(g_socket);
    g_socket = INVALID_SOCKET;
    WSACleanup();
    return 0;
}
