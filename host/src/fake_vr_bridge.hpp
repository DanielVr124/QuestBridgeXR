#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include "qbx/protocol.hpp"

struct FakeVrSettings {
    bool enabled = false;
    std::string allowedProcesses = "RobloxPlayerBeta.exe";
    bool yawEnabled = true;
    bool pitchEnabled = true;
    bool invertPitch = false;
    float sensitivityPxPerRadian = 850.0f;
    float pitchSensitivity = 0.9f;
    float deadzoneRadians = 0.0015f;
    int maxDeltaPixels = 70;
};

class FakeVrBridge {
public:
    bool load(const std::string& path);
    void pollHotkeys();
    void onTracking(const qbx::TrackingPayload& tracking);
    bool enabled() const { return settings_.enabled; }
    bool active() const { return active_; }
    const FakeVrSettings& settings() const { return settings_; }

private:
    bool foregroundProcessAllowed() const;
    void resetReference() { haveReference_ = false; }
    static float wrapAngle(float radians);
    static void orientationToYawPitch(const float q[4], float& yaw, float& pitch);
    FakeVrSettings settings_{};
    std::vector<std::string> allowedProcessNames_;
    bool active_ = false;
    bool haveReference_ = false;
    bool previousF8Down_ = false;
    bool previousF9Down_ = false;
    bool previousEscDown_ = false;
    float previousYaw_ = 0.0f;
    float previousPitch_ = 0.0f;
};
