#include "ambient_audio.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr unsigned int kSampleRate = 44100;
constexpr unsigned int kFramesPerBuffer = 2048;
constexpr float kPi = 3.14159265358979323846f;
enum Profile { kDream = 0, kInterior = 1, kWater = 2, kWind = 3, kRain = 4, kCozy = 5, kCosmic = 6 };

float clampSample(float value) { return std::clamp(value, -1.0f, 1.0f); }
}

AmbientAudio::~AmbientAudio() { shutdown(); }

bool AmbientAudio::initialize(bool enabled) {
    if (initialized_) { setEnabled(enabled); return true; }
    enabled_.store(enabled);
    SLresult result = slCreateEngine(&engineObject_, 0, nullptr, 0, nullptr, nullptr);
    if (result != SL_RESULT_SUCCESS || !engineObject_) return false;
    if ((*engineObject_)->Realize(engineObject_, SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS) { shutdown(); return false; }
    if ((*engineObject_)->GetInterface(engineObject_, SL_IID_ENGINE, &engine_) != SL_RESULT_SUCCESS) { shutdown(); return false; }
    if ((*engine_)->CreateOutputMix(engine_, &outputMixObject_, 0, nullptr, nullptr) != SL_RESULT_SUCCESS) { shutdown(); return false; }
    if ((*outputMixObject_)->Realize(outputMixObject_, SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS) { shutdown(); return false; }

    SLDataLocator_AndroidSimpleBufferQueue queueLocator = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2 };
    SLDataFormat_PCM pcm = { SL_DATAFORMAT_PCM, 2, SL_SAMPLINGRATE_44_1,
        SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
        SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT, SL_BYTEORDER_LITTLEENDIAN };
    SLDataSource source = { &queueLocator, &pcm };
    SLDataLocator_OutputMix sinkLocator = { SL_DATALOCATOR_OUTPUTMIX, outputMixObject_ };
    SLDataSink sink = { &sinkLocator, nullptr };
    const SLInterfaceID ids[] = { SL_IID_ANDROIDSIMPLEBUFFERQUEUE };
    const SLboolean required[] = { SL_BOOLEAN_TRUE };
    result = (*engine_)->CreateAudioPlayer(engine_, &playerObject_, &source, &sink, 1, ids, required);
    if (result != SL_RESULT_SUCCESS || !playerObject_) { shutdown(); return false; }
    if ((*playerObject_)->Realize(playerObject_, SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS) { shutdown(); return false; }
    if ((*playerObject_)->GetInterface(playerObject_, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &queue_) != SL_RESULT_SUCCESS) { shutdown(); return false; }
    if ((*playerObject_)->GetInterface(playerObject_, SL_IID_PLAY, &play_) != SL_RESULT_SUCCESS) { shutdown(); return false; }
    if ((*queue_)->RegisterCallback(queue_, queueCallback, this) != SL_RESULT_SUCCESS) { shutdown(); return false; }
    for (unsigned int i=0;i<buffers_.size();++i) {
        renderBuffer(buffers_[i]);
        if ((*queue_)->Enqueue(queue_, buffers_[i].data(), sizeof(buffers_[i])) != SL_RESULT_SUCCESS) { shutdown(); return false; }
    }
    nextBuffer_ = 0;
    if ((*play_)->SetPlayState(play_, SL_PLAYSTATE_PLAYING) != SL_RESULT_SUCCESS) { shutdown(); return false; }
    initialized_ = true;
    return true;
}

void AmbientAudio::shutdown() {
    enabled_.store(false);
    if (play_) (*play_)->SetPlayState(play_, SL_PLAYSTATE_STOPPED);
    if (queue_) (*queue_)->Clear(queue_);
    if (playerObject_) (*playerObject_)->Destroy(playerObject_);
    if (outputMixObject_) (*outputMixObject_)->Destroy(outputMixObject_);
    if (engineObject_) (*engineObject_)->Destroy(engineObject_);
    queue_ = nullptr; play_ = nullptr; playerObject_ = nullptr;
    outputMixObject_ = nullptr; engine_ = nullptr; engineObject_ = nullptr;
    initialized_ = false;
}

void AmbientAudio::setEnvironment(const std::string& name) {
    int profile = kDream;
    if (name.find("backrooms") != std::string::npos || name.find("liminal") != std::string::npos ||
        name.find("threshold") != std::string::npos || name.find("hall") != std::string::npos ||
        name.find("corridor") != std::string::npos || name.find("hotel") != std::string::npos ||
        name.find("office") != std::string::npos) profile = kInterior;
    else if (name.find("pool") != std::string::npos || name.find("water") != std::string::npos ||
             name.find("ocean") != std::string::npos || name.find("lagoon") != std::string::npos ||
             name.find("river") != std::string::npos || name.find("underwater") != std::string::npos) profile = kWater;
    else if (name.find("rain") != std::string::npos || name.find("storm") != std::string::npos ||
             name.find("city") != std::string::npos || name.find("street") != std::string::npos ||
             name.find("tokyo") != std::string::npos) profile = kRain;
    else if (name.find("forest") != std::string::npos || name.find("meadow") != std::string::npos ||
             name.find("garden") != std::string::npos || name.find("grove") != std::string::npos ||
             name.find("woodland") != std::string::npos) profile = kWind;
    else if (name.find("cabin") != std::string::npos || name.find("cafe") != std::string::npos ||
             name.find("fireplace") != std::string::npos || name.find("reading") != std::string::npos ||
             name.find("bedroom") != std::string::npos) profile = kCozy;
    else if (name.find("space") != std::string::npos || name.find("planet") != std::string::npos ||
             name.find("nebula") != std::string::npos || name.find("cosmic") != std::string::npos ||
             name.find("moon") != std::string::npos || name.find("star") != std::string::npos) profile = kCosmic;
    profile_.store(profile);
}

void AmbientAudio::setEnabled(bool enabled) {
    enabled_.store(enabled);
    if (play_ && initialized_) (*play_)->SetPlayState(play_, enabled ? SL_PLAYSTATE_PLAYING : SL_PLAYSTATE_PAUSED);
}

void AmbientAudio::queueCallback(SLAndroidSimpleBufferQueueItf, void* context) {
    if (context) static_cast<AmbientAudio*>(context)->fillNextBuffer();
}

void AmbientAudio::fillNextBuffer() {
    if (!initialized_ || !queue_) return;
    auto& buffer = buffers_[nextBuffer_ % buffers_.size()];
    nextBuffer_ = (nextBuffer_ + 1U) % static_cast<unsigned int>(buffers_.size());
    renderBuffer(buffer);
    (*queue_)->Enqueue(queue_, buffer.data(), sizeof(buffer));
}

void AmbientAudio::renderBuffer(std::array<int16_t, 4096>& buffer) {
    const int profile = profile_.load();
    const bool enabled = enabled_.load();
    const float noiseMix = profile == kWater ? 0.13f : profile == kRain ? 0.15f : profile == kCozy ? 0.06f : 0.035f;
    const float alpha = profile == kWater ? 0.018f : profile == kWind ? 0.008f : 0.035f;
    for (unsigned int frame=0;frame<kFramesPerBuffer;++frame) {
        noiseState_ = noiseState_ * 1664525U + 1013904223U;
        const float noise = (static_cast<float>((noiseState_ >> 8) & 0xffffU) / 32767.5f) - 1.0f;
        lowpass_ += alpha * (noise - lowpass_);
        const float tA = (profile == kInterior ? 53.0f : profile == kCosmic ? 39.0f : profile == kCozy ? 66.0f : 48.0f);
        const float tB = (profile == kRain ? 133.0f : profile == kWater ? 82.0f : profile == kWind ? 94.0f : 117.0f);
        phaseA_ += 2.0f * kPi * tA / static_cast<float>(kSampleRate);
        phaseB_ += 2.0f * kPi * tB / static_cast<float>(kSampleRate);
        if (phaseA_ > 2.0f*kPi) phaseA_ -= 2.0f*kPi;
        if (phaseB_ > 2.0f*kPi) phaseB_ -= 2.0f*kPi;
        float tone = std::sin(phaseA_) * (profile == kInterior ? 0.13f : profile == kCosmic ? 0.10f : 0.055f);
        tone += std::sin(phaseB_) * 0.022f;
        float texturedNoise = profile == kWater ? noise * 0.65f + lowpass_ * 0.55f :
                              profile == kRain ? noise * 0.65f + lowpass_ * 0.25f :
                              profile == kWind ? lowpass_ * 1.1f :
                              profile == kCozy ? noise * 0.23f + lowpass_ * 0.55f :
                              profile == kDream ? noise * 0.07f + lowpass_ * 0.25f : lowpass_ * 0.45f;
        const float slowMod = 0.75f + 0.25f * std::sin(phaseA_ * 0.13f);
        float sample = enabled ? (tone + texturedNoise * noiseMix) * slowMod : 0.0f;
        sample = clampSample(sample) * 4200.0f; // kept intentionally subtle
        const float stereo = std::sin(phaseB_ * 0.71f) * 180.0f;
        buffer[frame*2U] = static_cast<int16_t>(std::clamp(sample + stereo, -32768.0f, 32767.0f));
        buffer[frame*2U+1U] = static_cast<int16_t>(std::clamp(sample - stereo, -32768.0f, 32767.0f));
    }
}
