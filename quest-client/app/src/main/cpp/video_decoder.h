#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <GLES3/gl3.h>
#include <android/native_window.h>
#include <media/NdkImage.h>
#include <media/NdkImageReader.h>
#include <media/NdkMediaCodec.h>
#include <media/NdkMediaFormat.h>

struct QbxVideoConfig {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t framesPerSecond = 72;
    uint32_t bitrateBitsPerSecond = 0;
    uint16_t codec = 1;  // 1 = H.264, 2 = HEVC
    uint16_t layout = 1; // 0 = mono, 1 = side-by-side stereo
};

class VideoDecoder {
public:
    VideoDecoder() = default;
    ~VideoDecoder();
    VideoDecoder(const VideoDecoder&) = delete;
    VideoDecoder& operator=(const VideoDecoder&) = delete;

    bool initGl();
    void releaseGl();
    void setImageControls(float brightness, float contrast, float saturation, float gamma,
                          int aspectMode, bool flipHorizontal, bool flipVertical);
    void setSwapEyes(bool swapEyes) { swapEyes_ = swapEyes; }
    bool configure(const QbxVideoConfig& config);
    void pushFrame(const uint8_t* bytes, std::size_t size, uint64_t presentationTimeUs);
    void pumpOutput();
    void drawEye(int eyeIndex, int targetWidth, int targetHeight);
    GLuint renderEyeTexture(int eyeIndex, int targetWidth = 1024, int targetHeight = 576);
    bool hasFrame() const { return hasFrame_; }
    uint32_t width() const { return config_.width; }
    uint32_t height() const { return config_.height; }
    uint16_t layout() const { return config_.layout; }
    const char* status() const { return status_; }

private:
    void stopCodec();
    void uploadPlane(GLuint texture, int plane, const std::vector<uint8_t>& data, int width, int height);
    bool copyImagePlane(AImage* image, int planeIndex, int width, int height,
                        std::vector<uint8_t>& packed);
    GLuint compileShader(GLenum type, const char* source);

    QbxVideoConfig config_{};
    AMediaCodec* codec_ = nullptr;
    AImageReader* imageReader_ = nullptr;
    ANativeWindow* decoderWindow_ = nullptr; // owned by imageReader_
    GLuint shaderProgram_ = 0;
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint textures_[3] = {0, 0, 0};
    GLuint eyeOutputTextures_[2] = {0, 0};
    GLuint eyeOutputFramebuffers_[2] = {0, 0};
    int eyeOutputWidth_ = 0;
    int eyeOutputHeight_ = 0;
    int textureWidths_[3] = {0, 0, 0};
    int textureHeights_[3] = {0, 0, 0};
    GLint locY_ = -1;
    GLint locU_ = -1;
    GLint locV_ = -1;
    GLint locEye_ = -1;
    GLint locLayout_ = -1;
    GLint locBrightness_ = -1;
    GLint locContrast_ = -1;
    GLint locSaturation_ = -1;
    GLint locGamma_ = -1;
    GLint locAspectMode_ = -1;
    GLint locSourceAspect_ = -1;
    GLint locTargetAspect_ = -1;
    GLint locFlipHorizontal_ = -1;
    GLint locFlipVertical_ = -1;
    float brightness_ = 0.0f;
    float contrast_ = 1.0f;
    float saturation_ = 1.0f;
    float gamma_ = 1.0f;
    int aspectMode_ = 0; // 0 stretch, 1 fit, 2 crop
    bool flipHorizontal_ = false;
    bool flipVertical_ = false;
    bool swapEyes_ = false;
    bool glReady_ = false;
    bool configured_ = false;
    bool hasFrame_ = false;
    char status_[160] = "waiting for video config";
};
