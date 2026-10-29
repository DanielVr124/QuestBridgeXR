// QuestBridge XR experimental Windows OpenXR runtime.
// SPDX-License-Identifier: MIT
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D11
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <openxr/openxr_loader_negotiation.h>
#include "qbx/protocol.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <deque>
#include <mutex>
#include <memory>
#include <thread>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;
namespace {
constexpr uint32_t kViewWidth = 1440;
constexpr uint32_t kViewHeight = 1600;
constexpr uint32_t kMirrorOutputWidth = 1920;
constexpr uint32_t kMirrorOutputHeight = 1080;
constexpr int64_t kRefreshNs = 11'111'111; // 90 Hz initial fixed-rate prototype.
constexpr wchar_t kMirrorClass[] = L"QuestBridgeOpenXRMirrorClass";
constexpr wchar_t kMirrorTitle[] = L"QuestBridge XR Compositor";
constexpr wchar_t kRuntimeName[] = L"QuestBridge XR Experimental D3D11 Runtime";
constexpr uint16_t kTelemetryPort = 9945;

std::mutex g_mutex;
std::mutex g_socketStartMutex;
std::atomic<bool> g_wsaStarted{false};

struct PoseData { XrVector3f position{0, 0, 0}; XrQuaternionf orientation{0, 0, 0, 1}; };
struct ControllerData {
    qbx::ControllerPayload wire{};
    bool received{false};
    std::chrono::steady_clock::time_point receivedAt{};
};
struct TrackingData {
    qbx::TrackingPayload wire{};
    bool received{false};
    std::chrono::steady_clock::time_point receivedAt{};
    ControllerData controllers[2];
    SOCKET socket{INVALID_SOCKET};
    bool socketReady{false};
};
TrackingData g_tracking;

inline XrQuaternionf QMul(const XrQuaternionf& a, const XrQuaternionf& b) {
    return {a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y,
            a.w*b.y - a.x*b.z + a.y*b.w + a.z*b.x,
            a.w*b.z + a.x*b.y - a.y*b.x + a.z*b.w,
            a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z};
}
inline XrQuaternionf QInv(const XrQuaternionf& q) { return {-q.x, -q.y, -q.z, q.w}; }
inline XrVector3f QRotate(const XrQuaternionf& q, const XrVector3f& v) {
    const XrQuaternionf p{v.x, v.y, v.z, 0};
    const XrQuaternionf r = QMul(QMul(q, p), QInv(q));
    return {r.x, r.y, r.z};
}
inline PoseData PMul(const PoseData& a, const PoseData& b) {
    const XrVector3f moved = QRotate(a.orientation, b.position);
    return {{a.position.x + moved.x, a.position.y + moved.y, a.position.z + moved.z},
            QMul(a.orientation, b.orientation)};
}
inline PoseData PInv(const PoseData& p) {
    const auto q = QInv(p.orientation);
    const XrVector3f neg{-p.position.x, -p.position.y, -p.position.z};
    return {QRotate(q, neg), q};
}
inline int64_t NowNs() {
    LARGE_INTEGER counter{}, frequency{};
    QueryPerformanceCounter(&counter); QueryPerformanceFrequency(&frequency);
    return frequency.QuadPart ? static_cast<int64_t>((static_cast<long double>(counter.QuadPart) * 1'000'000'000.0L) / static_cast<long double>(frequency.QuadPart)) : 0;
}
inline bool IsFresh(std::chrono::steady_clock::time_point time, std::chrono::milliseconds maxAge = std::chrono::milliseconds(500)) {
    return time.time_since_epoch().count() != 0 && (std::chrono::steady_clock::now() - time) <= maxAge;
}
PoseData HeadPose() {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_tracking.received && IsFresh(g_tracking.receivedAt)) {
        const bool positionValid = (g_tracking.wire.trackingFlags & 0x1u) != 0;
        const bool orientationValid = (g_tracking.wire.trackingFlags & 0x2u) != 0;
        const XrVector3f position = positionValid
            ? XrVector3f{g_tracking.wire.headPosition[0], g_tracking.wire.headPosition[1], g_tracking.wire.headPosition[2]}
            : XrVector3f{0.0f, 1.65f, 0.0f};
        const XrQuaternionf orientation = orientationValid
            ? XrQuaternionf{g_tracking.wire.headOrientation[0], g_tracking.wire.headOrientation[1], g_tracking.wire.headOrientation[2], g_tracking.wire.headOrientation[3]}
            : XrQuaternionf{0, 0, 0, 1};
        return {position, orientation};
    }
    // Deterministic fallback makes API/lifecycle testing possible without a Quest connection.
    return {{0.0f, 1.65f, 0.0f}, {0, 0, 0, 1}};
}
PoseData ControllerPose(uint8_t hand, bool* valid = nullptr) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (hand > 1) hand = 0;
    const auto& c = g_tracking.controllers[hand];
    const bool ok = c.received && IsFresh(c.receivedAt) && ((c.wire.trackingFlags & 0x3u) == 0x3u);
    if (valid) *valid = ok;
    if (ok) return {{c.wire.position[0], c.wire.position[1], c.wire.position[2]},
                    {c.wire.orientation[0], c.wire.orientation[1], c.wire.orientation[2], c.wire.orientation[3]}};
    return {{hand == 0 ? -0.22f : 0.22f, 1.35f, -0.30f}, {0, 0, 0, 1}};
}
ControllerData ControllerSnapshot(uint8_t hand) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (hand > 1) hand = 0;
    return g_tracking.controllers[hand];
}

void StartTrackingFeed() {
    std::lock_guard<std::mutex> startupLock(g_socketStartMutex);
    if (g_tracking.socket != INVALID_SOCKET) return;
    if (!g_wsaStarted.exchange(true)) {
        WSADATA data{};
        if (WSAStartup(MAKEWORD(2, 2), &data) != 0) { g_wsaStarted.store(false); return; }
    }
    SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET) return;
    sockaddr_in addr{}; addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); addr.sin_port = htons(kTelemetryPort);
    if (bind(s, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        closesocket(s); return;
    }
    u_long nonBlocking = 1;
    ioctlsocket(s, FIONBIO, &nonBlocking);
    std::lock_guard<std::mutex> lock(g_mutex);
    g_tracking.socket = s; g_tracking.socketReady = true;
}
void PumpTrackingFeed() {
    StartTrackingFeed();
    SOCKET s;
    { std::lock_guard<std::mutex> lock(g_mutex); s = g_tracking.socket; }
    if (s == INVALID_SOCKET) return;
    alignas(qbx::PacketHeader) std::array<uint8_t, 2048> bytes{};
    for (;;) {
        sockaddr_in from{}; int fromLen = sizeof(from);
        const int n = recvfrom(s, reinterpret_cast<char*>(bytes.data()), static_cast<int>(bytes.size()), 0,
                               reinterpret_cast<sockaddr*>(&from), &fromLen);
        if (n == SOCKET_ERROR) {
            const int err = WSAGetLastError();
            if (err == WSAEWOULDBLOCK) break;
            break;
        }
        if (n < static_cast<int>(sizeof(qbx::PacketHeader))) continue;
        qbx::PacketHeader h{}; std::memcpy(&h, bytes.data(), sizeof(h));
        if (h.magic != qbx::kMagic || h.version != qbx::kProtocolVersion ||
            h.payloadBytes != static_cast<uint32_t>(n - static_cast<int>(sizeof(h)))) continue;
        const auto* payload = bytes.data() + sizeof(h);
        const auto now = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(g_mutex);
        if (h.type == static_cast<uint16_t>(qbx::PacketType::Tracking) && h.payloadBytes == sizeof(qbx::TrackingPayload)) {
            std::memcpy(&g_tracking.wire, payload, sizeof(g_tracking.wire));
            g_tracking.received = true; g_tracking.receivedAt = now;
        } else if (h.type == static_cast<uint16_t>(qbx::PacketType::Controller) && h.payloadBytes == sizeof(qbx::ControllerPayload)) {
            qbx::ControllerPayload c{}; std::memcpy(&c, payload, sizeof(c));
            if (c.hand <= 1) { g_tracking.controllers[c.hand].wire = c; g_tracking.controllers[c.hand].received = true; g_tracking.controllers[c.hand].receivedAt = now; }
        }
    }
}

} // namespace — close helpers; define the OpenXR opaque handles in global namespace.

struct XrInstance_T;
struct XrSession_T;
struct XrSpace_T;
struct XrSwapchain_T;
struct XrActionSet_T;
struct XrAction_T;

struct XrInstance_T {
    std::vector<XrSession> sessions;
    std::vector<XrActionSet> actionSets;
    std::vector<std::string> enabledExtensions;
    std::unordered_map<std::string, XrPath> pathByString;
    std::unordered_map<XrPath, std::string> stringByPath;
    XrPath nextPath{1};
    std::deque<XrEventDataBuffer> events;
    uint64_t systemId{1};
};
struct XrSession_T {
    XrInstance_T* instance{};
    XrSessionState state{XR_SESSION_STATE_IDLE};
    std::deque<XrEventDataBuffer> localEvents;
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    std::vector<XrSwapchain> swapchains;
    std::vector<XrSpace> spaces;
    std::vector<XrActionSet> actionSets;
    HWND mirrorWindow{};
    ComPtr<IDXGISwapChain> mirrorSwapchain;
    ComPtr<ID3D11VertexShader> mirrorVertexShader;
    ComPtr<ID3D11PixelShader> mirrorPixelShader;
    ComPtr<ID3D11SamplerState> mirrorSampler;
    ComPtr<ID3D11RasterizerState> mirrorRasterizer;
    ComPtr<ID3D11Buffer> mirrorUvBuffer;
    DXGI_FORMAT mirrorFormat{DXGI_FORMAT_UNKNOWN};
    uint32_t mirrorEyeWidth{kMirrorOutputWidth / 2};
    uint32_t mirrorHeight{kMirrorOutputHeight};
    bool begun{false};
    bool frameWaited{false};
    bool frameBegun{false};
    bool actionsAttached{false};
    bool exitRequested{false};
    uint64_t frameId{0};
    std::chrono::steady_clock::time_point nextFrameDeadline{};
};
struct XrSpace_T {
    XrSession_T* session{};
    bool isAction{false};
    XrReferenceSpaceType referenceType{XR_REFERENCE_SPACE_TYPE_LOCAL};
    XrPosef offset{{0,0,0,1},{0,0,0}};
    XrAction_T* action{};
    XrPath subactionPath{XR_NULL_PATH};
};
struct XrSwapchain_T {
    XrSession_T* session{};
    int64_t format{DXGI_FORMAT_R8G8B8A8_UNORM};
    uint32_t width{}, height{}, arraySize{1}, mipCount{1}, sampleCount{1};
    XrSwapchainUsageFlags usage{};
    std::vector<ComPtr<ID3D11Texture2D>> images;
    std::vector<std::vector<ComPtr<ID3D11ShaderResourceView>>> imageSrvs;
    uint32_t nextImage{0};
    uint32_t acquiredIndex{UINT32_MAX};
    uint32_t lastReleasedIndex{UINT32_MAX};
};
struct XrActionSet_T {
    XrInstance_T* instance{};
    XrSession_T* session{};
    std::string name;
    std::vector<XrAction> actions;
    std::array<bool, 2> activeHands{false, false};
};
struct ActionStateCache {
    bool active{false};
    bool booleanValue{false};
    float floatValue{0.0f};
    XrVector2f vectorValue{0.0f, 0.0f};
    bool poseActive{false};
    bool changed{false};
    XrTime lastChangeTime{0};
};
struct XrAction_T {
    XrActionSet_T* set{};
    std::string name;
    XrActionType type{XR_ACTION_TYPE_BOOLEAN_INPUT};
    std::vector<XrPath> subactionPaths;
    std::vector<XrPath> bindings;
    std::array<ActionStateCache, 2> cached{};
};

namespace {

XrPath StringToPathInternal(XrInstance_T* instance, const char* text) {
    if (!instance || !text || text[0] != '/') return XR_NULL_PATH;
    const auto found = instance->pathByString.find(text);
    if (found != instance->pathByString.end()) return found->second;
    const XrPath id = instance->nextPath++;
    instance->pathByString.emplace(text, id); instance->stringByPath.emplace(id, text);
    return id;
}
std::string PathString(XrInstance_T* instance, XrPath path) {
    if (!instance) return {};
    const auto it = instance->stringByPath.find(path);
    return it == instance->stringByPath.end() ? std::string{} : it->second;
}
int HandFromPath(XrInstance_T* instance, XrPath path) {
    const auto text = PathString(instance, path);
    if (text.find("/user/hand/left") != std::string::npos) return 0;
    if (text.find("/user/hand/right") != std::string::npos) return 1;
    return -1;
}
XrPath MakePath(XrInstance_T* instance, const char* str) { return StringToPathInternal(instance, str); }

void QueueSessionState(XrSession_T* session, XrSessionState state) {
    if (!session || !session->instance) return;
    XrEventDataSessionStateChanged changed{XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED};
    changed.session = reinterpret_cast<XrSession>(session); changed.time = NowNs(); changed.state = state;
    XrEventDataBuffer buffer{}; buffer.type = XR_TYPE_EVENT_DATA_BUFFER;
    std::memcpy(&buffer, &changed, sizeof(changed));
    session->instance->events.push_back(buffer);
    session->state = state;
}

LRESULT CALLBACK MirrorWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CLOSE: ShowWindow(hwnd, SW_MINIMIZE); return 0;
    case WM_ERASEBKGND: return 1;
    default: return DefWindowProcW(hwnd, msg, wp, lp);
    }
}
bool EnsureMirror(XrSession_T* session, uint32_t sourceWidth, uint32_t sourceHeight, DXGI_FORMAT format) {
    if (!session || !session->device || !sourceWidth || !sourceHeight) return false;
    (void)sourceWidth; (void)sourceHeight;
    if (session->mirrorSwapchain && session->mirrorFormat == format) return true;
    session->mirrorSwapchain.Reset();
    if (!session->mirrorWindow) {
        WNDCLASSEXW wc{}; wc.cbSize = sizeof(wc); wc.lpfnWndProc = MirrorWndProc;
        wc.hInstance = GetModuleHandleW(nullptr); wc.lpszClassName = kMirrorClass;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        RegisterClassExW(&wc); // ERROR_CLASS_ALREADY_EXISTS is expected after the first instance.
        session->mirrorWindow = CreateWindowExW(0, kMirrorClass, kMirrorTitle, WS_OVERLAPPEDWINDOW,
            80, 80, 1280, 720,
            nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
        if (!session->mirrorWindow) return false;
    }
    DXGI_SWAP_CHAIN_DESC desc{}; desc.BufferDesc.Width = kMirrorOutputWidth; desc.BufferDesc.Height = kMirrorOutputHeight;
    desc.BufferDesc.Format = format; desc.BufferDesc.RefreshRate.Numerator = 60; desc.BufferDesc.RefreshRate.Denominator = 1;
    desc.SampleDesc.Count = 1; desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2; desc.OutputWindow = session->mirrorWindow; desc.Windowed = TRUE;
    desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    ComPtr<IDXGIDevice> dxgiDevice; ComPtr<IDXGIAdapter> adapter; ComPtr<IDXGIFactory> factory;
    if (FAILED(session->device.As(&dxgiDevice)) || FAILED(dxgiDevice->GetAdapter(&adapter)) ||
        FAILED(adapter->GetParent(IID_PPV_ARGS(&factory))) ||
        FAILED(factory->CreateSwapChain(session->device.Get(), &desc, &session->mirrorSwapchain))) {
        session->mirrorSwapchain.Reset(); return false;
    }
    factory->MakeWindowAssociation(session->mirrorWindow, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES);

    static constexpr char shader[] = R"HLSL(
Texture2D eyeTexture : register(t0);
SamplerState eyeSampler : register(s0);
cbuffer UVTransform : register(b0) { float4 uvRect; };
struct VertexOut { float4 position : SV_Position; float2 uv : TEXCOORD0; };
VertexOut VSMain(uint id : SV_VertexID) {
    VertexOut o;
    float2 p[3] = { float2(-1.0, 1.0), float2(3.0, 1.0), float2(-1.0, -3.0) };
    float2 t[3] = { float2(0.0, 0.0), float2(2.0, 0.0), float2(0.0, 2.0) };
    o.position = float4(p[id], 0.0, 1.0); o.uv = t[id]; return o;
}
float4 PSMain(VertexOut i) : SV_Target { return eyeTexture.Sample(eyeSampler, i.uv * uvRect.zw + uvRect.xy); }
)HLSL";
    if (!session->mirrorVertexShader || !session->mirrorPixelShader) {
        ComPtr<ID3DBlob> vs, ps, errors;
        if (FAILED(D3DCompile(shader, sizeof(shader) - 1, "qbx_mirror.hlsl", nullptr, nullptr, "VSMain", "vs_4_0", 0, 0, &vs, &errors)) ||
            FAILED(D3DCompile(shader, sizeof(shader) - 1, "qbx_mirror.hlsl", nullptr, nullptr, "PSMain", "ps_4_0", 0, 0, &ps, &errors)) ||
            FAILED(session->device->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(), nullptr, &session->mirrorVertexShader)) ||
            FAILED(session->device->CreatePixelShader(ps->GetBufferPointer(), ps->GetBufferSize(), nullptr, &session->mirrorPixelShader))) return false;
        D3D11_SAMPLER_DESC samp{}; samp.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR; samp.AddressU = samp.AddressV = samp.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP; samp.MaxLOD = D3D11_FLOAT32_MAX;
        if (FAILED(session->device->CreateSamplerState(&samp, &session->mirrorSampler))) return false;
        D3D11_RASTERIZER_DESC rast{}; rast.FillMode = D3D11_FILL_SOLID; rast.CullMode = D3D11_CULL_NONE; rast.DepthClipEnable = TRUE;
        if (FAILED(session->device->CreateRasterizerState(&rast, &session->mirrorRasterizer))) return false;
        D3D11_BUFFER_DESC cb{}; cb.ByteWidth = 16; cb.Usage = D3D11_USAGE_DEFAULT; cb.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        if (FAILED(session->device->CreateBuffer(&cb, nullptr, &session->mirrorUvBuffer))) return false;
    }
    session->mirrorFormat = format; session->mirrorEyeWidth = kMirrorOutputWidth / 2; session->mirrorHeight = kMirrorOutputHeight;
    ShowWindow(session->mirrorWindow, SW_SHOW); UpdateWindow(session->mirrorWindow);
    return true;
}

void PumpWindowMessages() {
    MSG msg{};
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
}

PoseData SpaceWorldPose(XrSpace space, bool* tracked = nullptr) {
    if (!space || !space->session) { if (tracked) *tracked = false; return {}; }
    PoseData base{};
    bool valid = true;
    if (space->isAction && space->action) {
        int hand = HandFromPath(space->session->instance, space->subactionPath);
        if (hand < 0) hand = 0;
        bool controllerValid = false; base = ControllerPose(static_cast<uint8_t>(hand), &controllerValid);
        const auto& input = space->action->cached[static_cast<size_t>(hand)];
        valid = controllerValid && space->action->set->activeHands[static_cast<size_t>(hand)] && input.poseActive;
    } else if (space->referenceType == XR_REFERENCE_SPACE_TYPE_VIEW) {
        base = HeadPose();
        std::lock_guard<std::mutex> lock(g_mutex);
        valid = g_tracking.received && IsFresh(g_tracking.receivedAt) && ((g_tracking.wire.trackingFlags & 0x3u) == 0x3u);
    }
    if (tracked) *tracked = valid;
    const PoseData offset{{space->offset.position.x, space->offset.position.y, space->offset.position.z},
                          {space->offset.orientation.x, space->offset.orientation.y, space->offset.orientation.z, space->offset.orientation.w}};
    return PMul(base, offset);
}

void MirrorSubmittedFrame(XrSession_T* session, const XrFrameEndInfo* endInfo) {
    if (!session || !session->device || !endInfo || !endInfo->layerCount) return;
    const XrCompositionLayerProjection* projection = nullptr;
    for (uint32_t li = 0; li < endInfo->layerCount; ++li) {
        const auto* base = endInfo->layers[li];
        if (base && base->type == XR_TYPE_COMPOSITION_LAYER_PROJECTION) {
            projection = reinterpret_cast<const XrCompositionLayerProjection*>(base); break;
        }
    }
    if (!projection || projection->viewCount == 0 || !projection->views) return;
    const auto& first = projection->views[0];
    auto* firstSwap = reinterpret_cast<XrSwapchain_T*>(first.subImage.swapchain);
    if (!firstSwap || firstSwap->session != session) return;
    const uint32_t eyeW = static_cast<uint32_t>(std::max(1, first.subImage.imageRect.extent.width));
    const uint32_t eyeH = static_cast<uint32_t>(std::max(1, first.subImage.imageRect.extent.height));
    if (!EnsureMirror(session, eyeW, eyeH, static_cast<DXGI_FORMAT>(firstSwap->format))) return;
    ComPtr<ID3D11Texture2D> back;
    if (FAILED(session->mirrorSwapchain->GetBuffer(0, IID_PPV_ARGS(&back)))) return;
    ComPtr<ID3D11RenderTargetView> rtv;
    if (FAILED(session->device->CreateRenderTargetView(back.Get(), nullptr, &rtv))) return;
    const float clear[4] = {0.025f, 0.03f, 0.05f, 1.0f};
    session->context->OMSetRenderTargets(1, rtv.GetAddressOf(), nullptr);
    session->context->ClearRenderTargetView(rtv.Get(), clear);
    session->context->IASetInputLayout(nullptr);
    session->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    session->context->VSSetShader(session->mirrorVertexShader.Get(), nullptr, 0);
    session->context->PSSetShader(session->mirrorPixelShader.Get(), nullptr, 0);
    session->context->PSSetSamplers(0, 1, session->mirrorSampler.GetAddressOf());
    session->context->RSSetState(session->mirrorRasterizer.Get());
    bool copied = false;
    for (uint32_t vi = 0; vi < std::min(projection->viewCount, 2u); ++vi) {
        const auto& view = projection->views[vi];
        auto* swap = reinterpret_cast<XrSwapchain_T*>(view.subImage.swapchain);
        if (!swap || swap->session != session || swap->lastReleasedIndex >= swap->images.size() ||
            swap->lastReleasedIndex >= swap->imageSrvs.size()) continue;
        const uint32_t arrayIndex = view.subImage.imageArrayIndex;
        if (arrayIndex >= swap->arraySize || arrayIndex >= swap->imageSrvs[swap->lastReleasedIndex].size()) continue;
        ID3D11ShaderResourceView* srv = swap->imageSrvs[swap->lastReleasedIndex][arrayIndex].Get();
        if (!srv) continue;
        D3D11_TEXTURE2D_DESC srcDesc{}; swap->images[swap->lastReleasedIndex]->GetDesc(&srcDesc);
        const LONG left = view.subImage.imageRect.offset.x, top = view.subImage.imageRect.offset.y;
        const LONG right = left + view.subImage.imageRect.extent.width, bottom = top + view.subImage.imageRect.extent.height;
        if (left < 0 || top < 0 || right <= left || bottom <= top || right > static_cast<LONG>(srcDesc.Width) || bottom > static_cast<LONG>(srcDesc.Height)) continue;
        const float uv[4] = {static_cast<float>(left) / srcDesc.Width, static_cast<float>(top) / srcDesc.Height,
                             static_cast<float>(right - left) / srcDesc.Width, static_cast<float>(bottom - top) / srcDesc.Height};
        session->context->UpdateSubresource(session->mirrorUvBuffer.Get(), 0, nullptr, uv, 0, 0);
        session->context->PSSetConstantBuffers(0, 1, session->mirrorUvBuffer.GetAddressOf());
        session->context->PSSetShaderResources(0, 1, &srv);
        D3D11_VIEWPORT viewport{}; viewport.TopLeftX = static_cast<float>(vi * session->mirrorEyeWidth); viewport.TopLeftY = 0;
        viewport.Width = static_cast<float>(session->mirrorEyeWidth); viewport.Height = static_cast<float>(session->mirrorHeight); viewport.MinDepth = 0; viewport.MaxDepth = 1;
        session->context->RSSetViewports(1, &viewport);
        session->context->Draw(3, 0); copied = true;
        ID3D11ShaderResourceView* nullSrv = nullptr; session->context->PSSetShaderResources(0, 1, &nullSrv);
    }
    if (copied) session->mirrorSwapchain->Present(0, 0);
}

XrSession_T* AsSession(XrSession h) { return reinterpret_cast<XrSession_T*>(h); }
XrInstance_T* AsInstance(XrInstance h) { return reinterpret_cast<XrInstance_T*>(h); }
XrSpace_T* AsSpace(XrSpace h) { return reinterpret_cast<XrSpace_T*>(h); }
XrSwapchain_T* AsSwapchain(XrSwapchain h) { return reinterpret_cast<XrSwapchain_T*>(h); }
XrActionSet_T* AsActionSet(XrActionSet h) { return reinterpret_cast<XrActionSet_T*>(h); }
XrAction_T* AsAction(XrAction h) { return reinterpret_cast<XrAction_T*>(h); }

// OpenXR enumeration helper: countOutput is always populated, even when capacity is too small.
template <typename T>
XrResult Enumerate(uint32_t capacity, uint32_t* countOutput, T* output, const std::vector<T>& values) {
    if (!countOutput) return XR_ERROR_VALIDATION_FAILURE;
    *countOutput = static_cast<uint32_t>(values.size());
    if (capacity == 0) return XR_SUCCESS;
    if (!output) return XR_ERROR_VALIDATION_FAILURE;
    if (capacity < values.size()) return XR_ERROR_SIZE_INSUFFICIENT;
    std::copy(values.begin(), values.end(), output); return XR_SUCCESS;
}

bool ActionBelongsToHand(const XrAction_T* action, int hand) {
    if (!action || !action->set) return false;
    if (action->subactionPaths.empty()) return true;
    for (XrPath path : action->subactionPaths) {
        if (HandFromPath(action->set->instance, path) == hand) return true;
    }
    return false;
}
bool ActionHasBindingForHand(const XrAction_T* action, int hand) {
    if (!action || !action->set || action->bindings.empty()) return false;
    for (XrPath path : action->bindings) {
        const int boundHand = HandFromPath(action->set->instance, path);
        if (boundHand < 0 || boundHand == hand) return true;
    }
    return false;
}
bool ActionAllowsSubactionPath(const XrAction_T* action, XrPath path) {
    if (!action || !action->set) return false;
    if (path == XR_NULL_PATH) return true;
    if (HandFromPath(action->set->instance, path) < 0) return false;
    return std::find(action->subactionPaths.begin(), action->subactionPaths.end(), path) != action->subactionPaths.end();
}
std::string BindingName(XrAction_T* action, XrPath subactionPath, int hand) {
    if (!action || !action->set) return {};
    // Bindings identify concrete controls (/input/trigger/value etc.). A subaction
    // path identifies a hand, so prefer a concrete matching binding first.
    for (XrPath p : action->bindings) {
        const int boundHand = HandFromPath(action->set->instance, p);
        if (boundHand == hand || boundHand < 0) {
            const auto binding = PathString(action->set->instance, p);
            if (binding.find("/input/") != std::string::npos || binding.find("/pose") != std::string::npos)
                return binding;
        }
    }
    if (subactionPath != XR_NULL_PATH) return PathString(action->set->instance, subactionPath);
    return action->name;
}
bool HasPathToken(const std::string& s, const char* token) { return s.find(token) != std::string::npos; }
bool IsButtonDown(const ControllerData& c, const std::string& binding) {
    if (!c.received || !IsFresh(c.receivedAt)) return false;
    if (HasPathToken(binding, "/trigger/click") || HasPathToken(binding, "/select/click")) return c.wire.trigger >= 0.5f || (c.wire.buttons & (1u << 5));
    if (HasPathToken(binding, "/squeeze/click") || HasPathToken(binding, "/grip/click")) return c.wire.grip >= 0.5f || (c.wire.buttons & (1u << 4));
    if (HasPathToken(binding, "/thumbstick/click")) return (c.wire.buttons & (1u << 2)) != 0;
    if (HasPathToken(binding, "/thumbstick/left")) return c.wire.thumbstick[0] < -0.5f;
    if (HasPathToken(binding, "/thumbstick/right")) return c.wire.thumbstick[0] > 0.5f;
    if (HasPathToken(binding, "/thumbstick/up")) return c.wire.thumbstick[1] > 0.5f;
    if (HasPathToken(binding, "/thumbstick/down")) return c.wire.thumbstick[1] < -0.5f;
    if (HasPathToken(binding, "/menu/click")) return (c.wire.buttons & (1u << 3)) != 0;
    if (HasPathToken(binding, "/a/click") || HasPathToken(binding, "/x/click")) return (c.wire.buttons & (1u << 0)) != 0;
    if (HasPathToken(binding, "/b/click") || HasPathToken(binding, "/y/click")) return (c.wire.buttons & (1u << 1)) != 0;
    return false;
}
float FloatForAction(XrAction_T* action, XrPath subactionPath, int hand) {
    const auto name = BindingName(action, subactionPath, hand); const auto c = ControllerSnapshot(static_cast<uint8_t>(hand));
    if (!c.received || !IsFresh(c.receivedAt)) return 0.0f;
    if (HasPathToken(name, "/trigger/value") || HasPathToken(name, "trigger")) return c.wire.trigger;
    if (HasPathToken(name, "/squeeze/value") || HasPathToken(name, "/grip/value") || HasPathToken(name, "squeeze") || HasPathToken(name, "grip")) return c.wire.grip;
    if (HasPathToken(name, "/thumbstick/x")) return c.wire.thumbstick[0];
    if (HasPathToken(name, "/thumbstick/y")) return c.wire.thumbstick[1];
    return 0.0f;
}
XrVector2f VectorForAction(XrAction_T* action, XrPath subactionPath, int hand) {
    const auto name = BindingName(action, subactionPath, hand); const auto c = ControllerSnapshot(static_cast<uint8_t>(hand));
    if (c.received && IsFresh(c.receivedAt) && (HasPathToken(name, "thumbstick") || HasPathToken(name, "move") || HasPathToken(name, "vector2")))
        return {c.wire.thumbstick[0], c.wire.thumbstick[1]};
    return {0, 0};
}
bool ActionPoseActive(XrAction_T* action, XrPath subactionPath, int hand) {
    for (XrPath p : action->bindings) if (HasPathToken(PathString(action->set->instance, p), "/pose")) {
        if (subactionPath == XR_NULL_PATH || HandFromPath(action->set->instance, p) == hand) {
            bool valid = false; ControllerPose(static_cast<uint8_t>(hand), &valid); return valid;
        }
    }
    return false;
}

// Function prototypes - deliberately named qbxXr* to avoid exporting OpenXR entry points directly.
XrResult XRAPI_CALL qbxGetInstanceProcAddr(XrInstance, const char*, PFN_xrVoidFunction*);
XrResult XRAPI_CALL qbxEnumerateInstanceVersion(uint32_t*);
XrResult XRAPI_CALL qbxEnumerateInstanceExtensionProperties(const char*, uint32_t, uint32_t*, XrExtensionProperties*);
XrResult XRAPI_CALL qbxCreateInstance(const XrInstanceCreateInfo*, XrInstance*);
XrResult XRAPI_CALL qbxDestroyInstance(XrInstance);
XrResult XRAPI_CALL qbxGetInstanceProperties(XrInstance, XrInstanceProperties*);
XrResult XRAPI_CALL qbxGetSystem(XrInstance, const XrSystemGetInfo*, XrSystemId*);
XrResult XRAPI_CALL qbxGetSystemProperties(XrInstance, XrSystemId, XrSystemProperties*);
XrResult XRAPI_CALL qbxEnumerateViewConfigurations(XrInstance, XrSystemId, uint32_t, uint32_t*, XrViewConfigurationType*);
XrResult XRAPI_CALL qbxGetViewConfigurationProperties(XrInstance, XrSystemId, XrViewConfigurationType, XrViewConfigurationProperties*);
XrResult XRAPI_CALL qbxEnumerateViewConfigurationViews(XrInstance, XrSystemId, XrViewConfigurationType, uint32_t, uint32_t*, XrViewConfigurationView*);
XrResult XRAPI_CALL qbxEnumerateEnvironmentBlendModes(XrInstance, XrSystemId, XrViewConfigurationType, uint32_t, uint32_t*, XrEnvironmentBlendMode*);
XrResult XRAPI_CALL qbxCreateSession(XrInstance, const XrSessionCreateInfo*, XrSession*);
XrResult XRAPI_CALL qbxDestroySession(XrSession);
XrResult XRAPI_CALL qbxBeginSession(XrSession, const XrSessionBeginInfo*);
XrResult XRAPI_CALL qbxEndSession(XrSession);
XrResult XRAPI_CALL qbxRequestExitSession(XrSession);
XrResult XRAPI_CALL qbxPollEvent(XrInstance, XrEventDataBuffer*);
XrResult XRAPI_CALL qbxEnumerateReferenceSpaces(XrSession, uint32_t, uint32_t*, XrReferenceSpaceType*);
XrResult XRAPI_CALL qbxCreateReferenceSpace(XrSession, const XrReferenceSpaceCreateInfo*, XrSpace*);
XrResult XRAPI_CALL qbxCreateActionSpace(XrSession, const XrActionSpaceCreateInfo*, XrSpace*);
XrResult XRAPI_CALL qbxDestroySpace(XrSpace);
XrResult XRAPI_CALL qbxLocateSpace(XrSpace, XrSpace, XrTime, XrSpaceLocation*);
XrResult XRAPI_CALL qbxGetReferenceSpaceBoundsRect(XrSession, XrReferenceSpaceType, XrExtent2Df*);
XrResult XRAPI_CALL qbxLocateViews(XrSession, const XrViewLocateInfo*, XrViewState*, uint32_t, uint32_t*, XrView*);
XrResult XRAPI_CALL qbxEnumerateSwapchainFormats(XrSession, uint32_t, uint32_t*, int64_t*);
XrResult XRAPI_CALL qbxCreateSwapchain(XrSession, const XrSwapchainCreateInfo*, XrSwapchain*);
XrResult XRAPI_CALL qbxDestroySwapchain(XrSwapchain);
XrResult XRAPI_CALL qbxEnumerateSwapchainImages(XrSwapchain, uint32_t, uint32_t*, XrSwapchainImageBaseHeader*);
XrResult XRAPI_CALL qbxAcquireSwapchainImage(XrSwapchain, const XrSwapchainImageAcquireInfo*, uint32_t*);
XrResult XRAPI_CALL qbxWaitSwapchainImage(XrSwapchain, const XrSwapchainImageWaitInfo*);
XrResult XRAPI_CALL qbxReleaseSwapchainImage(XrSwapchain, const XrSwapchainImageReleaseInfo*);
XrResult XRAPI_CALL qbxWaitFrame(XrSession, const XrFrameWaitInfo*, XrFrameState*);
XrResult XRAPI_CALL qbxBeginFrame(XrSession, const XrFrameBeginInfo*);
XrResult XRAPI_CALL qbxEndFrame(XrSession, const XrFrameEndInfo*);
XrResult XRAPI_CALL qbxStringToPath(XrInstance, const char*, XrPath*);
XrResult XRAPI_CALL qbxPathToString(XrInstance, XrPath, uint32_t, uint32_t*, char*);
XrResult XRAPI_CALL qbxCreateActionSet(XrInstance, const XrActionSetCreateInfo*, XrActionSet*);
XrResult XRAPI_CALL qbxDestroyActionSet(XrActionSet);
XrResult XRAPI_CALL qbxCreateAction(XrActionSet, const XrActionCreateInfo*, XrAction*);
XrResult XRAPI_CALL qbxDestroyAction(XrAction);
XrResult XRAPI_CALL qbxSuggestInteractionProfileBindings(XrInstance, const XrInteractionProfileSuggestedBinding*);
XrResult XRAPI_CALL qbxAttachSessionActionSets(XrSession, const XrSessionActionSetsAttachInfo*);
XrResult XRAPI_CALL qbxSyncActions(XrSession, const XrActionsSyncInfo*);
XrResult XRAPI_CALL qbxGetActionStateBoolean(XrSession, const XrActionStateGetInfo*, XrActionStateBoolean*);
XrResult XRAPI_CALL qbxGetActionStateFloat(XrSession, const XrActionStateGetInfo*, XrActionStateFloat*);
XrResult XRAPI_CALL qbxGetActionStateVector2f(XrSession, const XrActionStateGetInfo*, XrActionStateVector2f*);
XrResult XRAPI_CALL qbxGetActionStatePose(XrSession, const XrActionStateGetInfo*, XrActionStatePose*);
XrResult XRAPI_CALL qbxEnumerateBoundSourcesForAction(XrSession, const XrBoundSourcesForActionEnumerateInfo*, uint32_t, uint32_t*, XrPath*);
XrResult XRAPI_CALL qbxGetCurrentInteractionProfile(XrSession, XrPath, XrInteractionProfileState*);
XrResult XRAPI_CALL qbxGetInputSourceLocalizedName(XrSession, const XrInputSourceLocalizedNameGetInfo*, uint32_t, uint32_t*, char*);
XrResult XRAPI_CALL qbxApplyHapticFeedback(XrSession, const XrHapticActionInfo*, const XrHapticBaseHeader*);
XrResult XRAPI_CALL qbxStopHapticFeedback(XrSession, const XrHapticActionInfo*);
XrResult XRAPI_CALL qbxGetD3D11GraphicsRequirementsKHR(XrInstance, XrSystemId, XrGraphicsRequirementsD3D11KHR*);
XrResult XRAPI_CALL qbxResultToString(XrInstance, XrResult, char[XR_MAX_RESULT_STRING_SIZE]);
XrResult XRAPI_CALL qbxStructureTypeToString(XrInstance, XrStructureType, char[XR_MAX_STRUCTURE_NAME_SIZE]);

#define QBX_ENTRY(exportName, fn) if (std::strcmp(name, exportName) == 0) { *function = reinterpret_cast<PFN_xrVoidFunction>(fn); return XR_SUCCESS; }

XrResult XRAPI_CALL qbxGetInstanceProcAddr(XrInstance instanceHandle, const char* name, PFN_xrVoidFunction* function) {
    if (!name || !function) return XR_ERROR_VALIDATION_FAILURE;
    *function = nullptr;
    QBX_ENTRY("xrGetInstanceProcAddr", qbxGetInstanceProcAddr)
    QBX_ENTRY("xrEnumerateInstanceVersion", qbxEnumerateInstanceVersion)
    QBX_ENTRY("xrEnumerateInstanceExtensionProperties", qbxEnumerateInstanceExtensionProperties)
    QBX_ENTRY("xrCreateInstance", qbxCreateInstance)
    // The loader can ask for these before an instance exists.
    QBX_ENTRY("xrDestroyInstance", qbxDestroyInstance)
    QBX_ENTRY("xrGetInstanceProperties", qbxGetInstanceProperties)
    QBX_ENTRY("xrGetSystem", qbxGetSystem)
    QBX_ENTRY("xrGetSystemProperties", qbxGetSystemProperties)
    QBX_ENTRY("xrEnumerateViewConfigurations", qbxEnumerateViewConfigurations)
    QBX_ENTRY("xrGetViewConfigurationProperties", qbxGetViewConfigurationProperties)
    QBX_ENTRY("xrEnumerateViewConfigurationViews", qbxEnumerateViewConfigurationViews)
    QBX_ENTRY("xrEnumerateEnvironmentBlendModes", qbxEnumerateEnvironmentBlendModes)
    QBX_ENTRY("xrCreateSession", qbxCreateSession)
    QBX_ENTRY("xrDestroySession", qbxDestroySession)
    QBX_ENTRY("xrBeginSession", qbxBeginSession)
    QBX_ENTRY("xrEndSession", qbxEndSession)
    QBX_ENTRY("xrRequestExitSession", qbxRequestExitSession)
    QBX_ENTRY("xrPollEvent", qbxPollEvent)
    QBX_ENTRY("xrEnumerateReferenceSpaces", qbxEnumerateReferenceSpaces)
    QBX_ENTRY("xrCreateReferenceSpace", qbxCreateReferenceSpace)
    QBX_ENTRY("xrCreateActionSpace", qbxCreateActionSpace)
    QBX_ENTRY("xrDestroySpace", qbxDestroySpace)
    QBX_ENTRY("xrLocateSpace", qbxLocateSpace)
    QBX_ENTRY("xrGetReferenceSpaceBoundsRect", qbxGetReferenceSpaceBoundsRect)
    QBX_ENTRY("xrLocateViews", qbxLocateViews)
    QBX_ENTRY("xrEnumerateSwapchainFormats", qbxEnumerateSwapchainFormats)
    QBX_ENTRY("xrCreateSwapchain", qbxCreateSwapchain)
    QBX_ENTRY("xrDestroySwapchain", qbxDestroySwapchain)
    QBX_ENTRY("xrEnumerateSwapchainImages", qbxEnumerateSwapchainImages)
    QBX_ENTRY("xrAcquireSwapchainImage", qbxAcquireSwapchainImage)
    QBX_ENTRY("xrWaitSwapchainImage", qbxWaitSwapchainImage)
    QBX_ENTRY("xrReleaseSwapchainImage", qbxReleaseSwapchainImage)
    QBX_ENTRY("xrWaitFrame", qbxWaitFrame)
    QBX_ENTRY("xrBeginFrame", qbxBeginFrame)
    QBX_ENTRY("xrEndFrame", qbxEndFrame)
    QBX_ENTRY("xrStringToPath", qbxStringToPath)
    QBX_ENTRY("xrPathToString", qbxPathToString)
    QBX_ENTRY("xrCreateActionSet", qbxCreateActionSet)
    QBX_ENTRY("xrDestroyActionSet", qbxDestroyActionSet)
    QBX_ENTRY("xrCreateAction", qbxCreateAction)
    QBX_ENTRY("xrDestroyAction", qbxDestroyAction)
    QBX_ENTRY("xrSuggestInteractionProfileBindings", qbxSuggestInteractionProfileBindings)
    QBX_ENTRY("xrAttachSessionActionSets", qbxAttachSessionActionSets)
    QBX_ENTRY("xrSyncActions", qbxSyncActions)
    QBX_ENTRY("xrGetActionStateBoolean", qbxGetActionStateBoolean)
    QBX_ENTRY("xrGetActionStateFloat", qbxGetActionStateFloat)
    QBX_ENTRY("xrGetActionStateVector2f", qbxGetActionStateVector2f)
    QBX_ENTRY("xrGetActionStatePose", qbxGetActionStatePose)
    QBX_ENTRY("xrEnumerateBoundSourcesForAction", qbxEnumerateBoundSourcesForAction)
    QBX_ENTRY("xrGetCurrentInteractionProfile", qbxGetCurrentInteractionProfile)
    QBX_ENTRY("xrGetInputSourceLocalizedName", qbxGetInputSourceLocalizedName)
    QBX_ENTRY("xrApplyHapticFeedback", qbxApplyHapticFeedback)
    QBX_ENTRY("xrStopHapticFeedback", qbxStopHapticFeedback)
    if (std::strcmp(name, "xrGetD3D11GraphicsRequirementsKHR") == 0) {
        auto* instance = AsInstance(instanceHandle);
        if (!instance) return XR_ERROR_HANDLE_INVALID;
        if (std::find(instance->enabledExtensions.begin(), instance->enabledExtensions.end(), XR_KHR_D3D11_ENABLE_EXTENSION_NAME) == instance->enabledExtensions.end())
            return XR_ERROR_FUNCTION_UNSUPPORTED;
        *function = reinterpret_cast<PFN_xrVoidFunction>(qbxGetD3D11GraphicsRequirementsKHR);
        return XR_SUCCESS;
    }
    QBX_ENTRY("xrResultToString", qbxResultToString)
    QBX_ENTRY("xrStructureTypeToString", qbxStructureTypeToString)
    (void)instanceHandle;
    return XR_ERROR_FUNCTION_UNSUPPORTED;
}
#undef QBX_ENTRY

XrResult XRAPI_CALL qbxEnumerateInstanceVersion(XrVersion* version) {
    if (!version) return XR_ERROR_VALIDATION_FAILURE;
    *version = XR_MAKE_VERSION(1, 0, 0);
    return XR_SUCCESS;
}

XrResult XRAPI_CALL qbxEnumerateInstanceExtensionProperties(const char* layerName, uint32_t capacity, uint32_t* count, XrExtensionProperties* props) {
    if (layerName && layerName[0]) return XR_ERROR_API_LAYER_NOT_PRESENT;
    if (!count) return XR_ERROR_VALIDATION_FAILURE;
    *count = 1;
    if (capacity == 0) return XR_SUCCESS;
    if (!props) return XR_ERROR_VALIDATION_FAILURE;
    if (capacity < 1) return XR_ERROR_SIZE_INSUFFICIENT;
    if (props[0].type != XR_TYPE_EXTENSION_PROPERTIES) return XR_ERROR_VALIDATION_FAILURE;
    props[0].extensionVersion = XR_KHR_D3D11_enable_SPEC_VERSION;
    std::strncpy(props[0].extensionName, XR_KHR_D3D11_ENABLE_EXTENSION_NAME, XR_MAX_EXTENSION_NAME_SIZE - 1);
    props[0].extensionName[XR_MAX_EXTENSION_NAME_SIZE - 1] = '\0';
    return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxCreateInstance(const XrInstanceCreateInfo* info, XrInstance* output) {
    if (!info || !output || info->type != XR_TYPE_INSTANCE_CREATE_INFO) return XR_ERROR_VALIDATION_FAILURE;
    *output = XR_NULL_HANDLE;
    if (info->enabledExtensionCount && !info->enabledExtensionNames) return XR_ERROR_VALIDATION_FAILURE;
    if (info->enabledApiLayerCount && !info->enabledApiLayerNames) return XR_ERROR_VALIDATION_FAILURE;
    if (info->enabledApiLayerCount != 0) return XR_ERROR_API_LAYER_NOT_PRESENT;
    if (XR_VERSION_MAJOR(info->applicationInfo.apiVersion) != 1 || XR_VERSION_MINOR(info->applicationInfo.apiVersion) > 0)
        return XR_ERROR_API_VERSION_UNSUPPORTED;
    auto instance = std::make_unique<XrInstance_T>();
    for (uint32_t i = 0; i < info->enabledExtensionCount; ++i) {
        const char* ext = info->enabledExtensionNames[i];
        if (!ext || std::strcmp(ext, XR_KHR_D3D11_ENABLE_EXTENSION_NAME) != 0) return XR_ERROR_EXTENSION_NOT_PRESENT;
        instance->enabledExtensions.emplace_back(ext);
    }
    // Common paths are assigned deterministic handles early for predictable input behavior.
    MakePath(instance.get(), "/interaction_profiles/khr/simple_controller");
    MakePath(instance.get(), "/user/hand/left"); MakePath(instance.get(), "/user/hand/right");
    auto* ptr = instance.release(); *output = reinterpret_cast<XrInstance>(ptr); return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxDestroyInstance(XrInstance handle) {
    auto* instance = AsInstance(handle); if (!instance) return XR_ERROR_HANDLE_INVALID;
    if (!instance->sessions.empty() || !instance->actionSets.empty()) return XR_ERROR_CALL_ORDER_INVALID;
    delete instance; return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxGetInstanceProperties(XrInstance handle, XrInstanceProperties* props) {
    if (!AsInstance(handle) || !props || props->type != XR_TYPE_INSTANCE_PROPERTIES) return XR_ERROR_VALIDATION_FAILURE;
    props->runtimeVersion = XR_MAKE_VERSION(1, 0, 0);
    std::strncpy(props->runtimeName, "QuestBridge XR Experimental Runtime", XR_MAX_RUNTIME_NAME_SIZE - 1);
    props->runtimeName[XR_MAX_RUNTIME_NAME_SIZE - 1] = '\0'; return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxGetSystem(XrInstance handle, const XrSystemGetInfo* info, XrSystemId* id) {
    if (!AsInstance(handle) || !info || !id || info->type != XR_TYPE_SYSTEM_GET_INFO) return XR_ERROR_VALIDATION_FAILURE;
    if (info->formFactor != XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY) return XR_ERROR_FORM_FACTOR_UNSUPPORTED;
    *id = 1; return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxGetSystemProperties(XrInstance handle, XrSystemId id, XrSystemProperties* props) {
    if (!AsInstance(handle) || id != 1 || !props || props->type != XR_TYPE_SYSTEM_PROPERTIES) return XR_ERROR_VALIDATION_FAILURE;
    props->systemId = 1; props->vendorId = 0x514258; // QBX, project-defined vendor ID.
    std::strncpy(props->systemName, "QuestBridge XR Virtual HMD", XR_MAX_SYSTEM_NAME_SIZE - 1);
    props->systemName[XR_MAX_SYSTEM_NAME_SIZE - 1] = '\0';
    props->graphicsProperties.maxSwapchainImageWidth = 8192; props->graphicsProperties.maxSwapchainImageHeight = 8192;
    props->graphicsProperties.maxLayerCount = 1; // Only the projection layer is composited in this bring-up.
    props->trackingProperties.orientationTracking = XR_TRUE; props->trackingProperties.positionTracking = XR_TRUE;
    return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxEnumerateViewConfigurations(XrInstance handle, XrSystemId id, uint32_t cap, uint32_t* count, XrViewConfigurationType* values) {
    if (!AsInstance(handle) || id != 1) return XR_ERROR_VALIDATION_FAILURE;
    return Enumerate(cap, count, values, std::vector<XrViewConfigurationType>{XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO});
}
XrResult XRAPI_CALL qbxGetViewConfigurationProperties(XrInstance handle, XrSystemId id, XrViewConfigurationType type, XrViewConfigurationProperties* props) {
    if (!AsInstance(handle) || id != 1 || type != XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO || !props || props->type != XR_TYPE_VIEW_CONFIGURATION_PROPERTIES) return XR_ERROR_VALIDATION_FAILURE;
    props->viewConfigurationType = type; props->fovMutable = XR_FALSE; return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxEnumerateViewConfigurationViews(XrInstance handle, XrSystemId id, XrViewConfigurationType type, uint32_t cap, uint32_t* count, XrViewConfigurationView* views) {
    if (!AsInstance(handle) || id != 1 || type != XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO || !count) return XR_ERROR_VALIDATION_FAILURE;
    *count = 2; if (cap == 0) return XR_SUCCESS; if (!views) return XR_ERROR_VALIDATION_FAILURE; if (cap < 2) return XR_ERROR_SIZE_INSUFFICIENT;
    for (uint32_t i = 0; i < 2; ++i) {
        if (views[i].type != XR_TYPE_VIEW_CONFIGURATION_VIEW) return XR_ERROR_VALIDATION_FAILURE;
        views[i].recommendedImageRectWidth = kViewWidth; views[i].maxImageRectWidth = 4096;
        views[i].recommendedImageRectHeight = kViewHeight; views[i].maxImageRectHeight = 4096;
        views[i].recommendedSwapchainSampleCount = 1; views[i].maxSwapchainSampleCount = 1;
    }
    return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxEnumerateEnvironmentBlendModes(XrInstance handle, XrSystemId id, XrViewConfigurationType type, uint32_t cap, uint32_t* count, XrEnvironmentBlendMode* modes) {
    if (!AsInstance(handle) || id != 1 || type != XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO) return XR_ERROR_VALIDATION_FAILURE;
    return Enumerate(cap, count, modes, std::vector<XrEnvironmentBlendMode>{XR_ENVIRONMENT_BLEND_MODE_OPAQUE});
}

XrResult XRAPI_CALL qbxCreateSession(XrInstance instHandle, const XrSessionCreateInfo* info, XrSession* output) {
    auto* inst = AsInstance(instHandle);
    if (!inst || !info || info->type != XR_TYPE_SESSION_CREATE_INFO || !output || info->systemId != 1) return XR_ERROR_VALIDATION_FAILURE;
    *output = XR_NULL_HANDLE;
    const XrGraphicsBindingD3D11KHR* binding = nullptr;
    for (auto* node = reinterpret_cast<const XrBaseInStructure*>(info->next); node; node = node->next) {
        if (node->type == XR_TYPE_GRAPHICS_BINDING_D3D11_KHR) { binding = reinterpret_cast<const XrGraphicsBindingD3D11KHR*>(node); break; }
    }
    if (std::find(inst->enabledExtensions.begin(), inst->enabledExtensions.end(), XR_KHR_D3D11_ENABLE_EXTENSION_NAME) == inst->enabledExtensions.end())
        return XR_ERROR_EXTENSION_NOT_PRESENT;
    if (!binding || !binding->device || binding->device->GetFeatureLevel() < D3D_FEATURE_LEVEL_11_0) return XR_ERROR_GRAPHICS_DEVICE_INVALID;
    auto session = std::make_unique<XrSession_T>(); session->instance = inst; session->device = binding->device;
    session->device->GetImmediateContext(&session->context);
    
    auto* ptr = session.release(); inst->sessions.push_back(reinterpret_cast<XrSession>(ptr));
    StartTrackingFeed(); QueueSessionState(ptr, XR_SESSION_STATE_READY);
    *output = reinterpret_cast<XrSession>(ptr); return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxDestroySession(XrSession handle) {
    auto* s = AsSession(handle); if (!s) return XR_ERROR_HANDLE_INVALID;
    if (s->begun) return XR_ERROR_SESSION_RUNNING;
    if (!s->spaces.empty() || !s->swapchains.empty()) return XR_ERROR_CALL_ORDER_INVALID;
    for (auto* setHandle : s->actionSets) { auto* set = AsActionSet(setHandle); if (set) set->session = nullptr; }
    s->actionSets.clear();
    if (s->mirrorWindow) DestroyWindow(s->mirrorWindow);
    auto* inst = s->instance;
    auto& sessions = inst->sessions; sessions.erase(std::remove(sessions.begin(), sessions.end(), handle), sessions.end());
    delete s; return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxBeginSession(XrSession handle, const XrSessionBeginInfo* info) {
    auto* s = AsSession(handle); if (!s || !info || info->type != XR_TYPE_SESSION_BEGIN_INFO) return XR_ERROR_VALIDATION_FAILURE;
    if (s->begun) return XR_ERROR_SESSION_RUNNING;
    if (s->state != XR_SESSION_STATE_READY) return XR_ERROR_SESSION_NOT_READY;
    if (info->primaryViewConfigurationType != XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO) return XR_ERROR_VIEW_CONFIGURATION_TYPE_UNSUPPORTED;
    s->begun = true; s->nextFrameDeadline = std::chrono::steady_clock::now(); QueueSessionState(s, XR_SESSION_STATE_SYNCHRONIZED); QueueSessionState(s, XR_SESSION_STATE_VISIBLE); QueueSessionState(s, XR_SESSION_STATE_FOCUSED); return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxEndSession(XrSession handle) {
    auto* s = AsSession(handle); if (!s) return XR_ERROR_HANDLE_INVALID; if (!s->begun) return XR_ERROR_SESSION_NOT_RUNNING;
    if (s->state != XR_SESSION_STATE_STOPPING) return XR_ERROR_SESSION_NOT_STOPPING;
    s->begun = false; s->frameWaited = false; s->frameBegun = false; QueueSessionState(s, XR_SESSION_STATE_IDLE); s->exitRequested = false; return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxRequestExitSession(XrSession handle) {
    auto* s = AsSession(handle); if (!s) return XR_ERROR_HANDLE_INVALID; if (!s->begun) return XR_ERROR_SESSION_NOT_RUNNING; s->exitRequested = true;
    if (s->begun) QueueSessionState(s, XR_SESSION_STATE_STOPPING); return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxPollEvent(XrInstance handle, XrEventDataBuffer* eventData) {
    auto* inst = AsInstance(handle); if (!inst || !eventData || eventData->type != XR_TYPE_EVENT_DATA_BUFFER) return XR_ERROR_VALIDATION_FAILURE;
    PumpWindowMessages(); PumpTrackingFeed();
    if (inst->events.empty()) { eventData->type = XR_TYPE_EVENT_DATA_BUFFER; eventData->next = nullptr; return XR_EVENT_UNAVAILABLE; }
    *eventData = inst->events.front(); inst->events.pop_front(); return XR_SUCCESS;
}

XrResult XRAPI_CALL qbxEnumerateReferenceSpaces(XrSession handle, uint32_t cap, uint32_t* count, XrReferenceSpaceType* values) {
    if (!AsSession(handle)) return XR_ERROR_HANDLE_INVALID;
    return Enumerate(cap, count, values, std::vector<XrReferenceSpaceType>{XR_REFERENCE_SPACE_TYPE_VIEW, XR_REFERENCE_SPACE_TYPE_LOCAL, XR_REFERENCE_SPACE_TYPE_STAGE});
}
XrResult XRAPI_CALL qbxCreateReferenceSpace(XrSession handle, const XrReferenceSpaceCreateInfo* info, XrSpace* output) {
    auto* s = AsSession(handle); if (!s || !info || info->type != XR_TYPE_REFERENCE_SPACE_CREATE_INFO || !output) return XR_ERROR_VALIDATION_FAILURE;
    if (info->referenceSpaceType != XR_REFERENCE_SPACE_TYPE_VIEW && info->referenceSpaceType != XR_REFERENCE_SPACE_TYPE_LOCAL && info->referenceSpaceType != XR_REFERENCE_SPACE_TYPE_STAGE) return XR_ERROR_REFERENCE_SPACE_UNSUPPORTED;
    auto* space = new XrSpace_T(); space->session = s; space->referenceType = info->referenceSpaceType; space->offset = info->poseInReferenceSpace;
    s->spaces.push_back(reinterpret_cast<XrSpace>(space)); *output = reinterpret_cast<XrSpace>(space); return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxCreateActionSpace(XrSession handle, const XrActionSpaceCreateInfo* info, XrSpace* output) {
    auto* s = AsSession(handle); if (!s || !info || info->type != XR_TYPE_ACTION_SPACE_CREATE_INFO || !output) return XR_ERROR_VALIDATION_FAILURE;
    auto* action = AsAction(info->action); if (!action || action->set->session != s || action->type != XR_ACTION_TYPE_POSE_INPUT) return XR_ERROR_HANDLE_INVALID;
    if (info->subactionPath != XR_NULL_PATH) {
        const int hand = HandFromPath(s->instance, info->subactionPath);
        if (hand < 0) return XR_ERROR_PATH_UNSUPPORTED;
        if (!action->subactionPaths.empty()) {
            bool declared = false;
            for (XrPath path : action->subactionPaths) if (HandFromPath(s->instance, path) == hand) declared = true;
            if (!declared) return XR_ERROR_PATH_UNSUPPORTED;
        }
    }
    auto* space = new XrSpace_T(); space->session = s; space->isAction = true; space->action = action; space->subactionPath = info->subactionPath; space->offset = info->poseInActionSpace;
    s->spaces.push_back(reinterpret_cast<XrSpace>(space)); *output = reinterpret_cast<XrSpace>(space); return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxDestroySpace(XrSpace handle) {
    auto* sp = AsSpace(handle); if (!sp || !sp->session) return XR_ERROR_HANDLE_INVALID;
    auto& v = sp->session->spaces; v.erase(std::remove(v.begin(), v.end(), handle), v.end()); delete sp; return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxLocateSpace(XrSpace targetHandle, XrSpace baseHandle, XrTime, XrSpaceLocation* location) {
    auto* target = AsSpace(targetHandle); auto* base = AsSpace(baseHandle);
    if (!target || !base || !location || location->type != XR_TYPE_SPACE_LOCATION || target->session != base->session) return XR_ERROR_VALIDATION_FAILURE;
    bool targetTracked = true; PoseData targetPose = SpaceWorldPose(target, &targetTracked); bool baseTracked = true; PoseData basePose = SpaceWorldPose(base, &baseTracked);
    const PoseData relative = PMul(PInv(basePose), targetPose);
    location->pose = {{relative.orientation.x, relative.orientation.y, relative.orientation.z, relative.orientation.w}, {relative.position.x, relative.position.y, relative.position.z}};
    location->locationFlags = 0;
    if (targetTracked && baseTracked) {
        location->locationFlags = XR_SPACE_LOCATION_ORIENTATION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT |
                                  XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_POSITION_TRACKED_BIT;
    }
    for (auto* out = reinterpret_cast<XrBaseOutStructure*>(location->next); out; out = out->next) {
        if (out->type == XR_TYPE_SPACE_VELOCITY) {
            auto* vel = reinterpret_cast<XrSpaceVelocity*>(out);
            // Velocities are not sampled/predicted by QBX v1; do not claim the zero placeholder is valid.
            vel->velocityFlags = 0; vel->linearVelocity = {0,0,0}; vel->angularVelocity = {0,0,0};
        }
    }
    return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxGetReferenceSpaceBoundsRect(XrSession handle, XrReferenceSpaceType type, XrExtent2Df* out) {
    if (!AsSession(handle) || !out) return XR_ERROR_VALIDATION_FAILURE;
    if (type != XR_REFERENCE_SPACE_TYPE_STAGE) return XR_SPACE_BOUNDS_UNAVAILABLE;
    // The current tracking packet carries poses but no guardian/play-area bounds.
    // Do not fabricate room boundaries; the caller can continue without bounds.
    return XR_SPACE_BOUNDS_UNAVAILABLE;
}
XrResult XRAPI_CALL qbxLocateViews(XrSession handle, const XrViewLocateInfo* info, XrViewState* state, uint32_t capacity, uint32_t* count, XrView* views) {
    auto* s = AsSession(handle); auto* base = info ? AsSpace(info->space) : nullptr;
    if (!s || !info || info->type != XR_TYPE_VIEW_LOCATE_INFO || !state || state->type != XR_TYPE_VIEW_STATE || !count || (capacity && !views) || !base || base->session != s || info->viewConfigurationType != XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO) return XR_ERROR_VALIDATION_FAILURE;
    *count = 2;
    state->viewStateFlags = 0;
    { std::lock_guard<std::mutex> lock(g_mutex);
      if (g_tracking.received && IsFresh(g_tracking.receivedAt)) {
          if (g_tracking.wire.trackingFlags & 0x2u) state->viewStateFlags |= XR_VIEW_STATE_ORIENTATION_VALID_BIT | XR_VIEW_STATE_ORIENTATION_TRACKED_BIT;
          if (g_tracking.wire.trackingFlags & 0x1u) state->viewStateFlags |= XR_VIEW_STATE_POSITION_VALID_BIT | XR_VIEW_STATE_POSITION_TRACKED_BIT;
      }
    }
    if (capacity == 0) return XR_SUCCESS; if (capacity < 2) return XR_ERROR_SIZE_INSUFFICIENT;
    const PoseData h = HeadPose(); PoseData basePose = SpaceWorldPose(base); const PoseData headInBase = PMul(PInv(basePose), h);
    for (uint32_t i = 0; i < 2; ++i) {
        if (views[i].type != XR_TYPE_VIEW) return XR_ERROR_VALIDATION_FAILURE;
        PoseData eye = PMul(headInBase, PoseData{{i == 0 ? -0.032f : 0.032f, 0, 0}, {0,0,0,1}});
        views[i].pose = {{eye.orientation.x, eye.orientation.y, eye.orientation.z, eye.orientation.w}, {eye.position.x, eye.position.y, eye.position.z}};
        views[i].fov = {-0.785398163f, 0.785398163f, 0.785398163f, -0.785398163f};
    }
    return XR_SUCCESS;
}

XrResult XRAPI_CALL qbxEnumerateSwapchainFormats(XrSession handle, uint32_t cap, uint32_t* count, int64_t* formats) {
    if (!AsSession(handle)) return XR_ERROR_HANDLE_INVALID;
    return Enumerate(cap, count, formats, std::vector<int64_t>{static_cast<int64_t>(DXGI_FORMAT_R8G8B8A8_UNORM)});
}
XrResult XRAPI_CALL qbxCreateSwapchain(XrSession handle, const XrSwapchainCreateInfo* info, XrSwapchain* output) {
    auto* s = AsSession(handle); if (!s || !info || info->type != XR_TYPE_SWAPCHAIN_CREATE_INFO || !output) return XR_ERROR_VALIDATION_FAILURE;
    if (info->width == 0 || info->height == 0 || info->width > 8192 || info->height > 8192 || info->arraySize == 0 || info->arraySize > 2 || info->mipCount != 1 || info->sampleCount != 1 || static_cast<DXGI_FORMAT>(info->format) != DXGI_FORMAT_R8G8B8A8_UNORM) return XR_ERROR_SWAPCHAIN_FORMAT_UNSUPPORTED;
    auto swap = std::make_unique<XrSwapchain_T>(); swap->session = s; swap->width = info->width; swap->height = info->height; swap->arraySize = info->arraySize; swap->mipCount = info->mipCount; swap->sampleCount = info->sampleCount; swap->usage = info->usageFlags; swap->format = info->format;
    D3D11_TEXTURE2D_DESC desc{}; desc.Width = info->width; desc.Height = info->height; desc.MipLevels = info->mipCount; desc.ArraySize = info->arraySize;
    desc.Format = static_cast<DXGI_FORMAT>(info->format); desc.SampleDesc.Count = info->sampleCount; desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE; desc.MiscFlags = 0;
    for (uint32_t i = 0; i < 3; ++i) {
        ComPtr<ID3D11Texture2D> img; if (FAILED(s->device->CreateTexture2D(&desc, nullptr, &img))) return XR_ERROR_RUNTIME_FAILURE;
        std::vector<ComPtr<ID3D11ShaderResourceView>> perArray;
        for (uint32_t slice = 0; slice < info->arraySize; ++slice) {
            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{}; srvDesc.Format = desc.Format;
            if (info->arraySize > 1) { srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY; srvDesc.Texture2DArray.MostDetailedMip = 0; srvDesc.Texture2DArray.MipLevels = 1; srvDesc.Texture2DArray.FirstArraySlice = slice; srvDesc.Texture2DArray.ArraySize = 1; }
            else { srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D; srvDesc.Texture2D.MostDetailedMip = 0; srvDesc.Texture2D.MipLevels = 1; }
            ComPtr<ID3D11ShaderResourceView> srv; if (FAILED(s->device->CreateShaderResourceView(img.Get(), &srvDesc, &srv))) return XR_ERROR_RUNTIME_FAILURE;
            perArray.push_back(std::move(srv));
        }
        swap->images.push_back(std::move(img)); swap->imageSrvs.push_back(std::move(perArray));
    }
    auto* ptr = swap.release(); s->swapchains.push_back(reinterpret_cast<XrSwapchain>(ptr)); *output = reinterpret_cast<XrSwapchain>(ptr);
    EnsureMirror(s, info->width, info->height, static_cast<DXGI_FORMAT>(info->format));
    return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxDestroySwapchain(XrSwapchain handle) {
    auto* sc = AsSwapchain(handle); if (!sc || !sc->session) return XR_ERROR_HANDLE_INVALID;
    auto& v = sc->session->swapchains; v.erase(std::remove(v.begin(), v.end(), handle), v.end()); delete sc; return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxEnumerateSwapchainImages(XrSwapchain handle, uint32_t cap, uint32_t* count, XrSwapchainImageBaseHeader* out) {
    auto* sc = AsSwapchain(handle); if (!sc || !count) return XR_ERROR_VALIDATION_FAILURE; *count = static_cast<uint32_t>(sc->images.size());
    if (cap == 0) return XR_SUCCESS; if (!out) return XR_ERROR_VALIDATION_FAILURE; if (cap < sc->images.size()) return XR_ERROR_SIZE_INSUFFICIENT;
    for (uint32_t i = 0; i < sc->images.size(); ++i) {
        if (out[i].type != XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR) return XR_ERROR_VALIDATION_FAILURE;
        auto* d3d = reinterpret_cast<XrSwapchainImageD3D11KHR*>(&out[i]); d3d->texture = sc->images[i].Get();
    }
    return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxAcquireSwapchainImage(XrSwapchain handle, const XrSwapchainImageAcquireInfo* info, uint32_t* index) {
    auto* sc = AsSwapchain(handle); if (!sc || !info || info->type != XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO || !index) return XR_ERROR_VALIDATION_FAILURE;
    if (sc->acquiredIndex != UINT32_MAX) return XR_ERROR_CALL_ORDER_INVALID;
    sc->acquiredIndex = sc->nextImage++ % static_cast<uint32_t>(sc->images.size()); *index = sc->acquiredIndex; return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxWaitSwapchainImage(XrSwapchain handle, const XrSwapchainImageWaitInfo* info) {
    auto* sc = AsSwapchain(handle); if (!sc || !info || info->type != XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO) return XR_ERROR_VALIDATION_FAILURE;
    return sc->acquiredIndex == UINT32_MAX ? XR_ERROR_CALL_ORDER_INVALID : XR_SUCCESS;
}
XrResult XRAPI_CALL qbxReleaseSwapchainImage(XrSwapchain handle, const XrSwapchainImageReleaseInfo* info) {
    auto* sc = AsSwapchain(handle); if (!sc || !info || info->type != XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO) return XR_ERROR_VALIDATION_FAILURE;
    if (sc->acquiredIndex == UINT32_MAX) return XR_ERROR_CALL_ORDER_INVALID;
    sc->lastReleasedIndex = sc->acquiredIndex; sc->acquiredIndex = UINT32_MAX; return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxWaitFrame(XrSession handle, const XrFrameWaitInfo* info, XrFrameState* state) {
    auto* s = AsSession(handle); if (!s || !info || info->type != XR_TYPE_FRAME_WAIT_INFO || !state || state->type != XR_TYPE_FRAME_STATE) return XR_ERROR_VALIDATION_FAILURE;
    if (!s->begun) return XR_ERROR_SESSION_NOT_RUNNING;
    if (s->frameWaited || s->frameBegun) return XR_ERROR_CALL_ORDER_INVALID;
    PumpTrackingFeed();
    const auto period = std::chrono::nanoseconds(kRefreshNs);
    const auto now = std::chrono::steady_clock::now();
    if (s->nextFrameDeadline < now - period) s->nextFrameDeadline = now;
    s->nextFrameDeadline += period;
    std::this_thread::sleep_until(s->nextFrameDeadline);
    state->predictedDisplayTime = NowNs() + kRefreshNs; state->predictedDisplayPeriod = kRefreshNs; state->shouldRender = s->exitRequested ? XR_FALSE : XR_TRUE; ++s->frameId; s->frameWaited = true; return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxBeginFrame(XrSession handle, const XrFrameBeginInfo* info) {
    auto* s = AsSession(handle); if (!s || !info || info->type != XR_TYPE_FRAME_BEGIN_INFO) return XR_ERROR_VALIDATION_FAILURE;
    if (!s->begun || s->frameBegun || !s->frameWaited) return XR_ERROR_CALL_ORDER_INVALID; s->frameWaited = false; s->frameBegun = true; return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxEndFrame(XrSession handle, const XrFrameEndInfo* info) {
    auto* s = AsSession(handle); if (!s || !info || info->type != XR_TYPE_FRAME_END_INFO) return XR_ERROR_VALIDATION_FAILURE;
    if (!s->begun || !s->frameBegun) return XR_ERROR_CALL_ORDER_INVALID;
    if (info->layerCount && !info->layers) return XR_ERROR_VALIDATION_FAILURE;
    if (info->environmentBlendMode != XR_ENVIRONMENT_BLEND_MODE_OPAQUE) return XR_ERROR_ENVIRONMENT_BLEND_MODE_UNSUPPORTED;
    if (info->layerCount > 1) return XR_ERROR_LAYER_LIMIT_EXCEEDED;
    MirrorSubmittedFrame(s, info); s->frameBegun = false; PumpWindowMessages();
    if (s->exitRequested && s->state != XR_SESSION_STATE_STOPPING) QueueSessionState(s, XR_SESSION_STATE_STOPPING);
    return XR_SUCCESS;
}

XrResult XRAPI_CALL qbxStringToPath(XrInstance handle, const char* text, XrPath* path) {
    auto* i = AsInstance(handle); if (!i || !text || !path || !text[0] || text[0] != '/') return XR_ERROR_PATH_FORMAT_INVALID;
    if (std::strlen(text) >= XR_MAX_PATH_LENGTH) return XR_ERROR_PATH_FORMAT_INVALID;
    *path = StringToPathInternal(i, text); return *path == XR_NULL_PATH ? XR_ERROR_PATH_FORMAT_INVALID : XR_SUCCESS;
}
XrResult XRAPI_CALL qbxPathToString(XrInstance handle, XrPath path, uint32_t cap, uint32_t* count, char* buffer) {
    auto* i = AsInstance(handle); if (!i || !count || path == XR_NULL_PATH) return XR_ERROR_VALIDATION_FAILURE;
    const std::string str = PathString(i, path); if (str.empty()) return XR_ERROR_PATH_INVALID;
    *count = static_cast<uint32_t>(str.size() + 1); if (cap == 0) return XR_SUCCESS; if (!buffer) return XR_ERROR_VALIDATION_FAILURE; if (cap < *count) return XR_ERROR_SIZE_INSUFFICIENT;
    std::memcpy(buffer, str.c_str(), *count); return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxCreateActionSet(XrInstance handle, const XrActionSetCreateInfo* info, XrActionSet* output) {
    auto* i = AsInstance(handle); if (!i || !info || info->type != XR_TYPE_ACTION_SET_CREATE_INFO || !output) return XR_ERROR_VALIDATION_FAILURE;
    if (!info->actionSetName || !info->localizedActionSetName || !info->actionSetName[0] || !info->localizedActionSetName[0]) return XR_ERROR_VALIDATION_FAILURE;
    if (std::strlen(info->actionSetName) >= XR_MAX_ACTION_SET_NAME_SIZE) return XR_ERROR_NAME_INVALID;
    if (std::strlen(info->localizedActionSetName) >= XR_MAX_LOCALIZED_ACTION_SET_NAME_SIZE) return XR_ERROR_LOCALIZED_NAME_INVALID;
    for (XrActionSet existingHandle : i->actionSets) {
        auto* existing = AsActionSet(existingHandle);
        if (existing && existing->name == info->actionSetName) return XR_ERROR_NAME_DUPLICATED;
    }
    auto* set = new XrActionSet_T(); set->instance = i; set->name = info->actionSetName;
    i->actionSets.push_back(reinterpret_cast<XrActionSet>(set)); *output = reinterpret_cast<XrActionSet>(set); return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxDestroyActionSet(XrActionSet handle) {
    auto* set = AsActionSet(handle); if (!set) return XR_ERROR_HANDLE_INVALID; if (set->session) return XR_ERROR_CALL_ORDER_INVALID;
    for (auto* a : set->actions) delete AsAction(a);
    auto& sets = set->instance->actionSets; sets.erase(std::remove(sets.begin(), sets.end(), handle), sets.end());
    delete set; return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxCreateAction(XrActionSet handle, const XrActionCreateInfo* info, XrAction* output) {
    auto* set = AsActionSet(handle); if (!set || !info || info->type != XR_TYPE_ACTION_CREATE_INFO || !output) return XR_ERROR_VALIDATION_FAILURE;
    if (set->session) return XR_ERROR_CALL_ORDER_INVALID;
    if (!info->actionName || !info->localizedActionName || !info->actionName[0] || !info->localizedActionName[0] || (info->countSubactionPaths && !info->subactionPaths)) return XR_ERROR_VALIDATION_FAILURE;
    if (std::strlen(info->actionName) >= XR_MAX_ACTION_NAME_SIZE) return XR_ERROR_NAME_INVALID;
    if (std::strlen(info->localizedActionName) >= XR_MAX_LOCALIZED_ACTION_NAME_SIZE) return XR_ERROR_LOCALIZED_NAME_INVALID;
    for (XrAction existingHandle : set->actions) {
        auto* existing = AsAction(existingHandle);
        if (existing && existing->name == info->actionName) return XR_ERROR_NAME_DUPLICATED;
    }
    const bool isInputAction = info->actionType >= XR_ACTION_TYPE_BOOLEAN_INPUT && info->actionType <= XR_ACTION_TYPE_POSE_INPUT;
    const bool isHapticAction = info->actionType == XR_ACTION_TYPE_VIBRATION_OUTPUT;
    if (!isInputAction && !isHapticAction) return XR_ERROR_ACTION_TYPE_MISMATCH;
    auto* a = new XrAction_T(); a->set = set; a->name = info->actionName; a->type = info->actionType;
    if (info->countSubactionPaths && info->subactionPaths) a->subactionPaths.assign(info->subactionPaths, info->subactionPaths + info->countSubactionPaths);
    set->actions.push_back(reinterpret_cast<XrAction>(a)); *output = reinterpret_cast<XrAction>(a); return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxDestroyAction(XrAction handle) {
    auto* a = AsAction(handle); if (!a || !a->set) return XR_ERROR_HANDLE_INVALID;
    if (a->set->session) return XR_ERROR_CALL_ORDER_INVALID;
    auto& v = a->set->actions; v.erase(std::remove(v.begin(), v.end(), handle), v.end()); delete a; return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxSuggestInteractionProfileBindings(XrInstance handle, const XrInteractionProfileSuggestedBinding* info) {
    auto* i = AsInstance(handle); if (!i || !info || info->type != XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING) return XR_ERROR_VALIDATION_FAILURE;
    if (info->countSuggestedBindings && !info->suggestedBindings) return XR_ERROR_VALIDATION_FAILURE;
    const std::string profile = PathString(i, info->interactionProfile);
    if (profile.find("/interaction_profiles/") != 0) return XR_ERROR_PATH_UNSUPPORTED;
    for (uint32_t b = 0; b < info->countSuggestedBindings; ++b) {
        auto* action = AsAction(info->suggestedBindings[b].action);
        if (!action || action->set->instance != i || info->suggestedBindings[b].binding == XR_NULL_PATH) return XR_ERROR_PATH_INVALID;
        if (std::find(action->bindings.begin(), action->bindings.end(), info->suggestedBindings[b].binding) == action->bindings.end()) action->bindings.push_back(info->suggestedBindings[b].binding);
    }
    return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxAttachSessionActionSets(XrSession handle, const XrSessionActionSetsAttachInfo* info) {
    auto* s = AsSession(handle); if (!s || !info || info->type != XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO ||
        info->countActionSets == 0 || !info->actionSets) return XR_ERROR_VALIDATION_FAILURE;
    if (s->actionsAttached) return XR_ERROR_ACTIONSETS_ALREADY_ATTACHED;
    for (uint32_t i = 0; i < info->countActionSets; ++i) {
        auto* set = AsActionSet(info->actionSets[i]); if (!set || set->instance != s->instance || (set->session && set->session != s)) return XR_ERROR_HANDLE_INVALID;
        set->session = s; if (std::find(s->actionSets.begin(), s->actionSets.end(), reinterpret_cast<XrActionSet>(set)) == s->actionSets.end()) s->actionSets.push_back(reinterpret_cast<XrActionSet>(set));
    }
    s->actionsAttached = true;
    return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxSyncActions(XrSession handle, const XrActionsSyncInfo* info) {
    auto* s = AsSession(handle);
    if (!s || !info || info->type != XR_TYPE_ACTIONS_SYNC_INFO ||
        (info->countActiveActionSets && !info->activeActionSets)) return XR_ERROR_VALIDATION_FAILURE;
    if (!s->actionsAttached) return XR_ERROR_ACTIONSET_NOT_ATTACHED;
    PumpTrackingFeed();
    for (XrActionSet setHandle : s->actionSets) {
        auto* set = AsActionSet(setHandle);
        if (set) set->activeHands = {false, false};
    }
    for (uint32_t i = 0; i < info->countActiveActionSets; ++i) {
        auto* set = AsActionSet(info->activeActionSets[i].actionSet);
        if (!set || set->session != s) return XR_ERROR_ACTIONSET_NOT_ATTACHED;
        const XrPath sub = info->activeActionSets[i].subactionPath;
        if (sub == XR_NULL_PATH) set->activeHands = {true, true};
        else {
            const int hand = HandFromPath(s->instance, sub);
            if (hand < 0) return XR_ERROR_PATH_INVALID;
            set->activeHands[static_cast<size_t>(hand)] = true;
        }
    }
    const XrPath handPaths[2] = {MakePath(s->instance, "/user/hand/left"), MakePath(s->instance, "/user/hand/right")};
    const XrTime now = NowNs();
    for (XrActionSet setHandle : s->actionSets) {
        auto* set = AsActionSet(setHandle); if (!set) continue;
        for (XrAction actionHandle : set->actions) {
            auto* action = AsAction(actionHandle); if (!action) continue;
            for (int hand = 0; hand < 2; ++hand) {
                auto& cache = action->cached[static_cast<size_t>(hand)];
                const bool active = set->activeHands[static_cast<size_t>(hand)] && ActionBelongsToHand(action, hand) && ActionHasBindingForHand(action, hand);
                bool booleanValue = false; float floatValue = 0.0f; XrVector2f vectorValue{0, 0}; bool poseActive = false;
                if (active) {
                    switch (action->type) {
                        case XR_ACTION_TYPE_BOOLEAN_INPUT:
                            booleanValue = IsButtonDown(ControllerSnapshot(static_cast<uint8_t>(hand)), BindingName(action, handPaths[hand], hand)); break;
                        case XR_ACTION_TYPE_FLOAT_INPUT:
                            floatValue = std::clamp(FloatForAction(action, handPaths[hand], hand), -1.0f, 1.0f); break;
                        case XR_ACTION_TYPE_VECTOR2F_INPUT:
                            vectorValue = VectorForAction(action, handPaths[hand], hand); break;
                        case XR_ACTION_TYPE_POSE_INPUT:
                            poseActive = ActionPoseActive(action, handPaths[hand], hand); break;
                        default: break;
                    }
                }
                const bool changed = cache.active != active || cache.booleanValue != booleanValue ||
                    std::fabs(cache.floatValue - floatValue) > 0.0001f ||
                    std::fabs(cache.vectorValue.x - vectorValue.x) > 0.0001f ||
                    std::fabs(cache.vectorValue.y - vectorValue.y) > 0.0001f || cache.poseActive != poseActive;
                cache.active = active; cache.booleanValue = booleanValue; cache.floatValue = floatValue;
                cache.vectorValue = vectorValue; cache.poseActive = poseActive; cache.changed = changed;
                if (changed) cache.lastChangeTime = now;
            }
        }
    }
    return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxGetActionStateBoolean(XrSession handle, const XrActionStateGetInfo* info, XrActionStateBoolean* state) {
    auto* s = AsSession(handle); auto* a = info ? AsAction(info->action) : nullptr;
    if (!s || !info || info->type != XR_TYPE_ACTION_STATE_GET_INFO || !state || state->type != XR_TYPE_ACTION_STATE_BOOLEAN || !a || a->set->session != s || a->type != XR_ACTION_TYPE_BOOLEAN_INPUT) return XR_ERROR_ACTION_TYPE_MISMATCH;
    if (!ActionAllowsSubactionPath(a, info->subactionPath)) return XR_ERROR_PATH_UNSUPPORTED;
    if (info->subactionPath != XR_NULL_PATH) {
        const int hand = HandFromPath(s->instance, info->subactionPath); if (hand < 0) return XR_ERROR_PATH_INVALID;
        const auto& c = a->cached[static_cast<size_t>(hand)]; state->currentState = c.booleanValue ? XR_TRUE : XR_FALSE;
        state->changedSinceLastSync = c.changed ? XR_TRUE : XR_FALSE; state->lastChangeTime = c.lastChangeTime; state->isActive = c.active ? XR_TRUE : XR_FALSE;
    } else {
        const auto& l = a->cached[0]; const auto& r = a->cached[1]; state->currentState = (l.booleanValue || r.booleanValue) ? XR_TRUE : XR_FALSE;
        state->changedSinceLastSync = (l.changed || r.changed) ? XR_TRUE : XR_FALSE; state->lastChangeTime = std::max(l.lastChangeTime, r.lastChangeTime);
        state->isActive = (l.active || r.active) ? XR_TRUE : XR_FALSE;
    }
    return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxGetActionStateFloat(XrSession handle, const XrActionStateGetInfo* info, XrActionStateFloat* state) {
    auto* s = AsSession(handle); auto* a = info ? AsAction(info->action) : nullptr;
    if (!s || !info || info->type != XR_TYPE_ACTION_STATE_GET_INFO || !state || state->type != XR_TYPE_ACTION_STATE_FLOAT || !a || a->set->session != s || a->type != XR_ACTION_TYPE_FLOAT_INPUT) return XR_ERROR_ACTION_TYPE_MISMATCH;
    if (!ActionAllowsSubactionPath(a, info->subactionPath)) return XR_ERROR_PATH_UNSUPPORTED;
    if (info->subactionPath != XR_NULL_PATH) {
        const int hand = HandFromPath(s->instance, info->subactionPath); if (hand < 0) return XR_ERROR_PATH_INVALID;
        const auto& c = a->cached[static_cast<size_t>(hand)]; state->currentState = c.floatValue; state->changedSinceLastSync = c.changed ? XR_TRUE : XR_FALSE;
        state->lastChangeTime = c.lastChangeTime; state->isActive = c.active ? XR_TRUE : XR_FALSE;
    } else {
        const auto& l = a->cached[0]; const auto& r = a->cached[1]; state->currentState = std::max(l.floatValue, r.floatValue);
        state->changedSinceLastSync = (l.changed || r.changed) ? XR_TRUE : XR_FALSE; state->lastChangeTime = std::max(l.lastChangeTime, r.lastChangeTime);
        state->isActive = (l.active || r.active) ? XR_TRUE : XR_FALSE;
    }
    return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxGetActionStateVector2f(XrSession handle, const XrActionStateGetInfo* info, XrActionStateVector2f* state) {
    auto* s = AsSession(handle); auto* a = info ? AsAction(info->action) : nullptr;
    if (!s || !info || info->type != XR_TYPE_ACTION_STATE_GET_INFO || !state || state->type != XR_TYPE_ACTION_STATE_VECTOR2F || !a || a->set->session != s || a->type != XR_ACTION_TYPE_VECTOR2F_INPUT) return XR_ERROR_ACTION_TYPE_MISMATCH;
    if (!ActionAllowsSubactionPath(a, info->subactionPath)) return XR_ERROR_PATH_UNSUPPORTED;
    if (info->subactionPath != XR_NULL_PATH) {
        const int hand = HandFromPath(s->instance, info->subactionPath); if (hand < 0) return XR_ERROR_PATH_INVALID;
        const auto& c = a->cached[static_cast<size_t>(hand)]; state->currentState = c.vectorValue; state->changedSinceLastSync = c.changed ? XR_TRUE : XR_FALSE;
        state->lastChangeTime = c.lastChangeTime; state->isActive = c.active ? XR_TRUE : XR_FALSE;
    } else {
        const auto& l = a->cached[0]; const auto& r = a->cached[1]; state->currentState = (l.vectorValue.x*l.vectorValue.x+l.vectorValue.y*l.vectorValue.y >= r.vectorValue.x*r.vectorValue.x+r.vectorValue.y*r.vectorValue.y) ? l.vectorValue : r.vectorValue;
        state->changedSinceLastSync = (l.changed || r.changed) ? XR_TRUE : XR_FALSE; state->lastChangeTime = std::max(l.lastChangeTime, r.lastChangeTime);
        state->isActive = (l.active || r.active) ? XR_TRUE : XR_FALSE;
    }
    return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxGetActionStatePose(XrSession handle, const XrActionStateGetInfo* info, XrActionStatePose* state) {
    auto* s = AsSession(handle); auto* a = info ? AsAction(info->action) : nullptr;
    if (!s || !info || info->type != XR_TYPE_ACTION_STATE_GET_INFO || !state || state->type != XR_TYPE_ACTION_STATE_POSE || !a || a->set->session != s || a->type != XR_ACTION_TYPE_POSE_INPUT) return XR_ERROR_ACTION_TYPE_MISMATCH;
    if (!ActionAllowsSubactionPath(a, info->subactionPath)) return XR_ERROR_PATH_UNSUPPORTED;
    if (info->subactionPath != XR_NULL_PATH) { const int hand = HandFromPath(s->instance, info->subactionPath); if (hand < 0) return XR_ERROR_PATH_INVALID; state->isActive = a->cached[static_cast<size_t>(hand)].poseActive ? XR_TRUE : XR_FALSE; }
    else state->isActive = (a->cached[0].poseActive || a->cached[1].poseActive) ? XR_TRUE : XR_FALSE;
    return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxEnumerateBoundSourcesForAction(XrSession handle, const XrBoundSourcesForActionEnumerateInfo* info, uint32_t cap, uint32_t* count, XrPath* paths) {
    auto* s = AsSession(handle); auto* a = info ? AsAction(info->action) : nullptr;
    if (!s || !info || info->type != XR_TYPE_BOUND_SOURCES_FOR_ACTION_ENUMERATE_INFO || !a || a->set->session != s) return XR_ERROR_VALIDATION_FAILURE;
    return Enumerate(cap, count, paths, a->bindings);
}
XrResult XRAPI_CALL qbxGetCurrentInteractionProfile(XrSession handle, XrPath topLevel, XrInteractionProfileState* state) {
    auto* s = AsSession(handle); if (!s || !state || state->type != XR_TYPE_INTERACTION_PROFILE_STATE || HandFromPath(s->instance, topLevel) < 0) return XR_ERROR_VALIDATION_FAILURE;
    state->interactionProfile = MakePath(s->instance, "/interaction_profiles/khr/simple_controller"); return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxGetInputSourceLocalizedName(XrSession handle, const XrInputSourceLocalizedNameGetInfo* info, uint32_t cap, uint32_t* count, char* buffer) {
    auto* s = AsSession(handle); if (!s || !info || info->type != XR_TYPE_INPUT_SOURCE_LOCALIZED_NAME_GET_INFO || !count) return XR_ERROR_VALIDATION_FAILURE;
    const int hand = HandFromPath(s->instance, info->sourcePath); if (hand < 0) return XR_ERROR_PATH_UNSUPPORTED;
    const char* name = hand == 0 ? "Left controller" : "Right controller"; *count = static_cast<uint32_t>(std::strlen(name) + 1);
    if (cap == 0) return XR_SUCCESS; if (!buffer) return XR_ERROR_VALIDATION_FAILURE; if (cap < *count) return XR_ERROR_SIZE_INSUFFICIENT; std::memcpy(buffer, name, *count); return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxApplyHapticFeedback(XrSession handle, const XrHapticActionInfo* info, const XrHapticBaseHeader* haptic) {
    auto* s = AsSession(handle); auto* action = info ? AsAction(info->action) : nullptr;
    if (!s || !info || info->type != XR_TYPE_HAPTIC_ACTION_INFO || !haptic || !action ||
        action->type != XR_ACTION_TYPE_VIBRATION_OUTPUT || action->set->session != s)
        return XR_ERROR_ACTION_TYPE_MISMATCH;
    // The protocol currently carries no haptic commands. Accept valid haptic actions but never claim that rumble occurred.
    return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxStopHapticFeedback(XrSession handle, const XrHapticActionInfo* info) {
    auto* s = AsSession(handle); auto* action = info ? AsAction(info->action) : nullptr;
    if (!s || !info || info->type != XR_TYPE_HAPTIC_ACTION_INFO || !action ||
        action->type != XR_ACTION_TYPE_VIBRATION_OUTPUT || action->set->session != s)
        return XR_ERROR_ACTION_TYPE_MISMATCH;
    return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxGetD3D11GraphicsRequirementsKHR(XrInstance handle, XrSystemId id, XrGraphicsRequirementsD3D11KHR* req) {
    auto* i = AsInstance(handle); if (!i || id != 1 || !req || req->type != XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR) return XR_ERROR_VALIDATION_FAILURE;
    req->minFeatureLevel = D3D_FEATURE_LEVEL_11_0;
    ComPtr<IDXGIFactory1> factory; ComPtr<IDXGIAdapter1> adapter;
    if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))) && SUCCEEDED(factory->EnumAdapters1(0, &adapter))) {
        DXGI_ADAPTER_DESC1 desc{}; if (SUCCEEDED(adapter->GetDesc1(&desc))) req->adapterLuid = desc.AdapterLuid;
    }
    return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxResultToString(XrInstance handle, XrResult result, char buffer[XR_MAX_RESULT_STRING_SIZE]) {
    if (!AsInstance(handle) || !buffer) return XR_ERROR_VALIDATION_FAILURE;
    const char* text = result == XR_SUCCESS ? "XR_SUCCESS" : result == XR_EVENT_UNAVAILABLE ? "XR_EVENT_UNAVAILABLE" : result == XR_ERROR_FUNCTION_UNSUPPORTED ? "XR_ERROR_FUNCTION_UNSUPPORTED" : "XR_ERROR (see OpenXR result code)";
    std::strncpy(buffer, text, XR_MAX_RESULT_STRING_SIZE - 1); buffer[XR_MAX_RESULT_STRING_SIZE - 1] = '\0'; return XR_SUCCESS;
}
XrResult XRAPI_CALL qbxStructureTypeToString(XrInstance handle, XrStructureType type, char buffer[XR_MAX_STRUCTURE_NAME_SIZE]) {
    if (!AsInstance(handle) || !buffer) return XR_ERROR_VALIDATION_FAILURE;
    const char* text = type == XR_TYPE_INSTANCE_CREATE_INFO ? "XR_TYPE_INSTANCE_CREATE_INFO" : type == XR_TYPE_SESSION_CREATE_INFO ? "XR_TYPE_SESSION_CREATE_INFO" : type == XR_TYPE_FRAME_END_INFO ? "XR_TYPE_FRAME_END_INFO" : "XR_TYPE (see OpenXR structure type)";
    std::strncpy(buffer, text, XR_MAX_STRUCTURE_NAME_SIZE - 1); buffer[XR_MAX_STRUCTURE_NAME_SIZE - 1] = '\0'; return XR_SUCCESS;
}
} // namespace

extern "C" __declspec(dllexport) XrResult XRAPI_CALL xrNegotiateLoaderRuntimeInterface(
    const XrNegotiateLoaderInfo* loaderInfo, XrNegotiateRuntimeRequest* runtimeRequest) {
    if (!loaderInfo || !runtimeRequest) return XR_ERROR_INITIALIZATION_FAILED;
    if (loaderInfo->structType != XR_LOADER_INTERFACE_STRUCT_LOADER_INFO ||
        loaderInfo->structVersion != XR_LOADER_INFO_STRUCT_VERSION ||
        loaderInfo->structSize != sizeof(XrNegotiateLoaderInfo) ||
        runtimeRequest->structType != XR_LOADER_INTERFACE_STRUCT_RUNTIME_REQUEST ||
        runtimeRequest->structVersion != XR_RUNTIME_INFO_STRUCT_VERSION ||
        runtimeRequest->structSize != sizeof(XrNegotiateRuntimeRequest)) return XR_ERROR_INITIALIZATION_FAILED;
    const XrVersion supportedRuntimeApi = XR_MAKE_VERSION(1, 0, 0);
    if (loaderInfo->minInterfaceVersion > 1 || loaderInfo->maxInterfaceVersion < 1 ||
        loaderInfo->minApiVersion > supportedRuntimeApi || loaderInfo->maxApiVersion < supportedRuntimeApi)
        return XR_ERROR_API_VERSION_UNSUPPORTED;
    runtimeRequest->runtimeInterfaceVersion = 1;
    runtimeRequest->runtimeApiVersion = supportedRuntimeApi;
    runtimeRequest->getInstanceProcAddr = qbxGetInstanceProcAddr;
    return XR_SUCCESS;
}
