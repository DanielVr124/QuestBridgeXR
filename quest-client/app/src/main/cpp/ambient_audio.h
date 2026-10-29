#pragma once

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <string>

class AmbientAudio {
public:
    AmbientAudio() = default;
    ~AmbientAudio();
    bool initialize(bool enabled);
    void shutdown();
    void setEnvironment(const std::string& environment);
    void setEnabled(bool enabled);
    AmbientAudio(const AmbientAudio&) = delete;
    AmbientAudio& operator=(const AmbientAudio&) = delete;

private:
    static void queueCallback(SLAndroidSimpleBufferQueueItf queue, void* context);
    void fillNextBuffer();
    void renderBuffer(std::array<int16_t, 4096>& buffer);

    SLObjectItf engineObject_ = nullptr;
    SLEngineItf engine_ = nullptr;
    SLObjectItf outputMixObject_ = nullptr;
    SLObjectItf playerObject_ = nullptr;
    SLPlayItf play_ = nullptr;
    SLAndroidSimpleBufferQueueItf queue_ = nullptr;
    std::array<std::array<int16_t, 4096>, 2> buffers_{};
    std::atomic<bool> enabled_{false};
    std::atomic<int> profile_{0};
    unsigned int nextBuffer_ = 0;
    uint32_t noiseState_ = 0xA341316Cu;
    float lowpass_ = 0.0f;
    float phaseA_ = 0.0f;
    float phaseB_ = 0.0f;
    bool initialized_ = false;
};
