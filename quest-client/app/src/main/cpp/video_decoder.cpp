#include "video_decoder.h"

#include <android/log.h>
#include <media/NdkImage.h>
#include <media/NdkImageReader.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>

#define QBX_LOG_TAG "QuestBridgeXR"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, QBX_LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, QBX_LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, QBX_LOG_TAG, __VA_ARGS__)

namespace {
const char* kVertexShader = R"GLSL(#version 300 es
precision highp float;
layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aUv;
out vec2 vUv;
void main() {
    gl_Position = vec4(aPosition, 0.0, 1.0);
    vUv = aUv;
}
)GLSL";

const char* kFragmentShader = R"GLSL(#version 300 es
precision highp float;
in vec2 vUv;
out vec4 outColor;
uniform sampler2D uY;
uniform sampler2D uU;
uniform sampler2D uV;
uniform int uEye;
uniform int uLayout;
uniform float uBrightness;
uniform float uContrast;
uniform float uSaturation;
uniform float uGamma;
uniform int uAspectMode;
uniform float uSourceAspect;
uniform float uTargetAspect;
uniform int uFlipHorizontal;
uniform int uFlipVertical;
void main() {
    vec2 eyeUv = vUv;
    if (uFlipHorizontal != 0) eyeUv.x = 1.0 - eyeUv.x;
    if (uFlipVertical != 0) eyeUv.y = 1.0 - eyeUv.y;
    if (uAspectMode == 1) {
        vec2 scale = vec2(1.0, 1.0);
        if (uSourceAspect > uTargetAspect) scale.y = uTargetAspect / uSourceAspect;
        else scale.x = uSourceAspect / uTargetAspect;
        vec2 p = (eyeUv - vec2(0.5)) / scale + vec2(0.5);
        if (p.x < 0.0 || p.x > 1.0 || p.y < 0.0 || p.y > 1.0) {
            outColor = vec4(0.0, 0.0, 0.0, 1.0);
            return;
        }
        eyeUv = p;
    } else if (uAspectMode == 2) {
        if (uSourceAspect > uTargetAspect) eyeUv.x = (eyeUv.x - 0.5) * (uTargetAspect / uSourceAspect) + 0.5;
        else eyeUv.y = (eyeUv.y - 0.5) * (uSourceAspect / uTargetAspect) + 0.5;
    }
    vec2 uv = eyeUv;
    if (uLayout == 1) {
        uv.x = uv.x * 0.5 + float(uEye) * 0.5;
    }
    float y = texture(uY, uv).r;
    float u = texture(uU, uv).r - 0.5;
    float v = texture(uV, uv).r - 0.5;
    // BT.709 limited-range YUV to RGB.
    y = 1.164383 * (y - 0.062745);
    vec3 rgb = vec3(y + 1.792741 * v,
                    y - 0.213249 * u - 0.532909 * v,
                    y + 2.112402 * u);
    rgb = (rgb - vec3(0.5)) * uContrast + vec3(0.5 + uBrightness);
    float luma = dot(rgb, vec3(0.2126, 0.7152, 0.0722));
    rgb = mix(vec3(luma), rgb, uSaturation);
    rgb = pow(max(rgb, vec3(0.0)), vec3(1.0 / uGamma));
    outColor = vec4(clamp(rgb, 0.0, 1.0), 1.0);
}
)GLSL";
}

VideoDecoder::~VideoDecoder() {
    stopCodec();
    releaseGl();
}

void VideoDecoder::setImageControls(float brightness, float contrast, float saturation, float gamma,
                                    int aspectMode, bool flipHorizontal, bool flipVertical) {
    brightness_ = std::clamp(brightness, -0.5f, 0.5f);
    contrast_ = std::clamp(contrast, 0.25f, 2.5f);
    saturation_ = std::clamp(saturation, 0.0f, 2.0f);
    gamma_ = std::clamp(gamma, 0.5f, 2.5f);
    aspectMode_ = std::clamp(aspectMode, 0, 2);
    flipHorizontal_ = flipHorizontal;
    flipVertical_ = flipVertical;
}

void VideoDecoder::releaseGl() {
    if (!glReady_) return;
    if (shaderProgram_) glDeleteProgram(shaderProgram_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (vbo_) glDeleteBuffers(1, &vbo_);
    glDeleteTextures(3, textures_);
    glDeleteTextures(2, eyeOutputTextures_);
    glDeleteFramebuffers(2, eyeOutputFramebuffers_);
    eyeOutputTextures_[0] = eyeOutputTextures_[1] = 0;
    eyeOutputFramebuffers_[0] = eyeOutputFramebuffers_[1] = 0;
    eyeOutputWidth_ = eyeOutputHeight_ = 0;
    shaderProgram_ = 0;
    vao_ = 0;
    vbo_ = 0;
    textures_[0] = textures_[1] = textures_[2] = 0;
    textureWidths_[0] = textureWidths_[1] = textureWidths_[2] = 0;
    textureHeights_[0] = textureHeights_[1] = textureHeights_[2] = 0;
    glReady_ = false;
}

GLuint VideoDecoder::compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
        char log[1024]{};
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        LOGE("Shader compile failed: %s", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool VideoDecoder::initGl() {
    if (glReady_) return true;
    const GLuint vs = compileShader(GL_VERTEX_SHADER, kVertexShader);
    const GLuint fs = compileShader(GL_FRAGMENT_SHADER, kFragmentShader);
    if (!vs || !fs) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        std::snprintf(status_, sizeof(status_), "video shader compile failed");
        return false;
    }
    shaderProgram_ = glCreateProgram();
    glAttachShader(shaderProgram_, vs);
    glAttachShader(shaderProgram_, fs);
    glLinkProgram(shaderProgram_);
    glDeleteShader(vs);
    glDeleteShader(fs);
    GLint linked = GL_FALSE;
    glGetProgramiv(shaderProgram_, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        char log[1024]{};
        glGetProgramInfoLog(shaderProgram_, sizeof(log), nullptr, log);
        LOGE("Video shader link failed: %s", log);
        glDeleteProgram(shaderProgram_);
        shaderProgram_ = 0;
        std::snprintf(status_, sizeof(status_), "video shader link failed");
        return false;
    }

    const float vertices[] = {
        // x, y, u, v; top-left uses texture row zero so decoded video is not upside-down.
        -1.f, -1.f, 0.f, 1.f,
         1.f, -1.f, 1.f, 1.f,
        -1.f,  1.f, 0.f, 0.f,
         1.f,  1.f, 1.f, 0.f,
    };
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
    glBindVertexArray(0);

    glGenTextures(3, textures_);
    for (GLuint texture : textures_) {
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    locY_ = glGetUniformLocation(shaderProgram_, "uY");
    locU_ = glGetUniformLocation(shaderProgram_, "uU");
    locV_ = glGetUniformLocation(shaderProgram_, "uV");
    locEye_ = glGetUniformLocation(shaderProgram_, "uEye");
    locLayout_ = glGetUniformLocation(shaderProgram_, "uLayout");
    locBrightness_ = glGetUniformLocation(shaderProgram_, "uBrightness");
    locContrast_ = glGetUniformLocation(shaderProgram_, "uContrast");
    locSaturation_ = glGetUniformLocation(shaderProgram_, "uSaturation");
    locGamma_ = glGetUniformLocation(shaderProgram_, "uGamma");
    locAspectMode_ = glGetUniformLocation(shaderProgram_, "uAspectMode");
    locSourceAspect_ = glGetUniformLocation(shaderProgram_, "uSourceAspect");
    locTargetAspect_ = glGetUniformLocation(shaderProgram_, "uTargetAspect");
    locFlipHorizontal_ = glGetUniformLocation(shaderProgram_, "uFlipHorizontal");
    locFlipVertical_ = glGetUniformLocation(shaderProgram_, "uFlipVertical");
    glReady_ = true;
    setImageControls(brightness_, contrast_, saturation_, gamma_, aspectMode_, flipHorizontal_, flipVertical_);
    return true;
}

void VideoDecoder::stopCodec() {
    if (codec_) {
        AMediaCodec_stop(codec_);
        AMediaCodec_delete(codec_);
        codec_ = nullptr;
    }
    configured_ = false;
    hasFrame_ = false;
    if (imageReader_) {
        AImageReader_delete(imageReader_);
        imageReader_ = nullptr;
        decoderWindow_ = nullptr; // owned by imageReader_; do not release separately
    }
}

bool VideoDecoder::configure(const QbxVideoConfig& config) {
    if (config.width < 64 || config.height < 64 || config.width > 4096 || config.height > 4096 ||
        config.framesPerSecond == 0 || config.framesPerSecond > 240 || config.layout > 1 ||
        (config.codec != 1 && config.codec != 2)) {
        std::snprintf(status_, sizeof(status_), "invalid video config");
        return false;
    }
    if (configured_ && config.width == config_.width && config.height == config_.height &&
        config.codec == config_.codec && config.layout == config_.layout &&
        config.framesPerSecond == config_.framesPerSecond) {
        return true;
    }
    stopCodec();
    config_ = config;
    const char* mime = config.codec == 1 ? "video/avc" : "video/hevc";
    const media_status_t readerStatus = AImageReader_new(static_cast<int32_t>(config.width),
        static_cast<int32_t>(config.height), AIMAGE_FORMAT_YUV_420_888, 3, &imageReader_);
    if (readerStatus != AMEDIA_OK || !imageReader_) {
        if (imageReader_) AImageReader_delete(imageReader_);
        imageReader_ = nullptr;
        std::snprintf(status_, sizeof(status_), "AImageReader creation failed (%d)", static_cast<int>(readerStatus));
        return false;
    }
    if (AImageReader_getWindow(imageReader_, &decoderWindow_) != AMEDIA_OK || !decoderWindow_) {
        AImageReader_delete(imageReader_);
        imageReader_ = nullptr;
        decoderWindow_ = nullptr;
        std::snprintf(status_, sizeof(status_), "AImageReader output surface unavailable");
        return false;
    }
    codec_ = AMediaCodec_createDecoderByType(mime);
    if (!codec_) {
        AImageReader_delete(imageReader_);
        imageReader_ = nullptr;
        decoderWindow_ = nullptr;
        std::snprintf(status_, sizeof(status_), "MediaCodec decoder unavailable for %s", mime);
        return false;
    }
    AMediaFormat* format = AMediaFormat_new();
    AMediaFormat_setString(format, AMEDIAFORMAT_KEY_MIME, mime);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_WIDTH, static_cast<int32_t>(config.width));
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_HEIGHT, static_cast<int32_t>(config.height));
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_FRAME_RATE, static_cast<int32_t>(config.framesPerSecond));
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_MAX_INPUT_SIZE,
                          static_cast<int32_t>(std::min<uint64_t>(config.width * uint64_t(config.height) * 2, 32u * 1024u * 1024u)));
    const media_status_t cfg = AMediaCodec_configure(codec_, format, decoderWindow_, nullptr, 0);
    AMediaFormat_delete(format);
    if (cfg != AMEDIA_OK || AMediaCodec_start(codec_) != AMEDIA_OK) {
        LOGE("MediaCodec configure/start failed status=%d", static_cast<int>(cfg));
        stopCodec();
        std::snprintf(status_, sizeof(status_), "MediaCodec configure failed");
        return false;
    }
    configured_ = true;
    hasFrame_ = false;
    std::snprintf(status_, sizeof(status_), "decoding %ux%u %s", config.width, config.height, mime);
    LOGI("Video decoder configured: %s %ux%u layout=%u fps=%u", mime, config.width, config.height,
         config.layout, config.framesPerSecond);
    return true;
}

void VideoDecoder::pushFrame(const uint8_t* bytes, std::size_t size, uint64_t presentationTimeUs) {
    if (!configured_ || !codec_ || !bytes || size == 0) return;
    const ssize_t index = AMediaCodec_dequeueInputBuffer(codec_, 0);
    if (index < 0) return; // Keep the render loop non-blocking; a later frame will arrive shortly.
    size_t capacity = 0;
    uint8_t* target = AMediaCodec_getInputBuffer(codec_, static_cast<size_t>(index), &capacity);
    if (!target || size > capacity) {
        AMediaCodec_queueInputBuffer(codec_, static_cast<size_t>(index), 0, 0, presentationTimeUs, 0);
        LOGW("Dropped encoded frame: input buffer too small (%zu > %zu)", size, capacity);
        return;
    }
    std::memcpy(target, bytes, size);
    if (AMediaCodec_queueInputBuffer(codec_, static_cast<size_t>(index), 0, size, presentationTimeUs, 0) != AMEDIA_OK) {
        LOGW("MediaCodec rejected encoded frame");
    }
}

bool VideoDecoder::copyImagePlane(AImage* image, int planeIndex, int width, int height,
                                  std::vector<uint8_t>& packed) {
    uint8_t* source = nullptr;
    int dataLength = 0;
    int32_t rowStride = 0;
    int32_t pixelStride = 0;
    if (AImage_getPlaneData(image, planeIndex, &source, &dataLength) != AMEDIA_OK || !source ||
        AImage_getPlaneRowStride(image, planeIndex, &rowStride) != AMEDIA_OK ||
        AImage_getPlanePixelStride(image, planeIndex, &pixelStride) != AMEDIA_OK ||
        rowStride <= 0 || pixelStride <= 0 || dataLength <= 0) {
        return false;
    }
    packed.resize(static_cast<size_t>(width) * height);
    for (int y = 0; y < height; ++y) {
        const int64_t rowOffset = static_cast<int64_t>(y) * rowStride;
        if (rowOffset >= dataLength) return false;
        for (int x = 0; x < width; ++x) {
            const int64_t offset = rowOffset + static_cast<int64_t>(x) * pixelStride;
            if (offset >= dataLength) return false;
            packed[static_cast<size_t>(y) * width + x] = source[offset];
        }
    }
    return true;
}

void VideoDecoder::uploadPlane(GLuint texture, int plane, const std::vector<uint8_t>& data, int width, int height) {
    glBindTexture(GL_TEXTURE_2D, texture);
    const GLenum internalFormat = GL_R8;
    if (textureWidths_[plane] != width || textureHeights_[plane] != height) {
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data.data());
        textureWidths_[plane] = width;
        textureHeights_[plane] = height;
    } else {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED, GL_UNSIGNED_BYTE, data.data());
    }
}

void VideoDecoder::pumpOutput() {
    if (!configured_ || !codec_ || !imageReader_ || !glReady_) return;

    // Release decoded output buffers to the CPU-readable AImageReader surface.
    // Drain several ready buffers, then copy only the newest image to reduce latency.
    for (int i = 0; i < 8; ++i) {
        AMediaCodecBufferInfo info{};
        const ssize_t index = AMediaCodec_dequeueOutputBuffer(codec_, &info, 0);
        if (index < 0) break; // includes TRY_AGAIN_LATER / output-format notifications
        const media_status_t released = AMediaCodec_releaseOutputBuffer(codec_, static_cast<size_t>(index), true);
        if (released != AMEDIA_OK) LOGW("MediaCodec releaseOutputBuffer(render) failed: %d", static_cast<int>(released));
    }

    AImage* image = nullptr;
    const media_status_t acquireStatus = AImageReader_acquireLatestImage(imageReader_, &image);
    if (acquireStatus != AMEDIA_OK || !image) return;

    int32_t w = 0;
    int32_t h = 0;
    bool copied = AImage_getWidth(image, &w) == AMEDIA_OK &&
                  AImage_getHeight(image, &h) == AMEDIA_OK &&
                  w == static_cast<int32_t>(config_.width) &&
                  h == static_cast<int32_t>(config_.height);
    int32_t planeCount = 0;
    if (copied) copied = AImage_getNumberOfPlanes(image, &planeCount) == AMEDIA_OK && planeCount >= 3;

    std::vector<uint8_t> y, u, v;
    if (copied) copied = copyImagePlane(image, 0, w, h, y);
    if (copied) copied = copyImagePlane(image, 1, w / 2, h / 2, u);
    if (copied) copied = copyImagePlane(image, 2, w / 2, h / 2, v);
    AImage_delete(image);

    if (copied) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        uploadPlane(textures_[0], 0, y, w, h);
        uploadPlane(textures_[1], 1, u, w / 2, h / 2);
        uploadPlane(textures_[2], 2, v, w / 2, h / 2);
        glBindTexture(GL_TEXTURE_2D, 0);
        hasFrame_ = true;
    } else {
        LOGW("Could not unpack an AImageReader YUV output image");
    }
}

GLuint VideoDecoder::renderEyeTexture(int eyeIndex, int targetWidth, int targetHeight) {
    if (!glReady_ || eyeIndex < 0 || eyeIndex > 1 || targetWidth <= 0 || targetHeight <= 0) return 0;
    if (!eyeOutputTextures_[0] || eyeOutputWidth_ != targetWidth || eyeOutputHeight_ != targetHeight) {
        if (eyeOutputTextures_[0]) glDeleteTextures(2, eyeOutputTextures_);
        if (eyeOutputFramebuffers_[0]) glDeleteFramebuffers(2, eyeOutputFramebuffers_);
        glGenTextures(2, eyeOutputTextures_);
        glGenFramebuffers(2, eyeOutputFramebuffers_);
        for (int i = 0; i < 2; ++i) {
            glBindTexture(GL_TEXTURE_2D, eyeOutputTextures_[i]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, targetWidth, targetHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glBindFramebuffer(GL_FRAMEBUFFER, eyeOutputFramebuffers_[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, eyeOutputTextures_[i], 0);
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                LOGE("Video eye output framebuffer incomplete");
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glBindTexture(GL_TEXTURE_2D, 0);
                return 0;
            }
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        eyeOutputWidth_ = targetWidth;
        eyeOutputHeight_ = targetHeight;
    }
    GLint oldFramebuffer = 0, oldViewport[4] = {};
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFramebuffer);
    glGetIntegerv(GL_VIEWPORT, oldViewport);
    glBindFramebuffer(GL_FRAMEBUFFER, eyeOutputFramebuffers_[eyeIndex]);
    glViewport(0, 0, targetWidth, targetHeight);
    glClearColor(0.015f, 0.02f, 0.035f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    drawEye(eyeIndex, targetWidth, targetHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(oldFramebuffer));
    glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);
    return eyeOutputTextures_[eyeIndex];
}

void VideoDecoder::drawEye(int eyeIndex, int targetWidth, int targetHeight) {
    if (!glReady_ || !hasFrame_) return;
    glUseProgram(shaderProgram_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textures_[0]);
    glUniform1i(locY_, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textures_[1]);
    glUniform1i(locU_, 1);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, textures_[2]);
    glUniform1i(locV_, 2);
    const int selectedEye = swapEyes_ ? 1 - (eyeIndex == 0 ? 0 : 1) : (eyeIndex == 0 ? 0 : 1);
    glUniform1i(locEye_, selectedEye);
    glUniform1i(locLayout_, config_.layout);
    glUniform1f(locBrightness_, brightness_);
    glUniform1f(locContrast_, contrast_);
    glUniform1f(locSaturation_, saturation_);
    glUniform1f(locGamma_, gamma_);
    glUniform1i(locAspectMode_, aspectMode_);
    const float sourceWidth = config_.layout == 1 ? static_cast<float>(config_.width) * 0.5f : static_cast<float>(config_.width);
    const float sourceAspect = config_.height > 0 ? sourceWidth / static_cast<float>(config_.height) : 1.0f;
    const float targetAspect = targetHeight > 0 ? static_cast<float>(targetWidth) / static_cast<float>(targetHeight) : sourceAspect;
    glUniform1f(locSourceAspect_, sourceAspect);
    glUniform1f(locTargetAspect_, targetAspect);
    glUniform1i(locFlipHorizontal_, flipHorizontal_ ? 1 : 0);
    glUniform1i(locFlipVertical_, flipVertical_ ? 1 : 0);
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}
