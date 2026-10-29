#pragma once

#include <cstdint>
#include <string>
#include <array>
#include <GLES3/gl3.h>
#include <android/asset_manager.h>
#include <openxr/openxr.h>

struct RoomSettings {
    std::string environment = "blossom";       // 88 scenes; see Studio Virtual Room settings
    std::string roomSize = "medium";           // small | medium | large
    std::string screenDestination = "monitor"; // monitor | tv | both
    int monitorCount = 1;                       // 1..3, replicated stream
    bool showTv = true;
    bool showCouch = true;
    bool showChair = true;
    bool roomControlsEnabled = true;
    bool sittingToggleEnabled = true;
    float sittingHeadDropM = 0.42f;
    bool smoothTurn = false;
    bool comfortVignette = true;
    float movementSpeed = 1.35f;
    float turnSpeed = 1.8f;
    float snapTurnDegrees = 30.0f;
    float floorOffsetY = -1.60f;
};

class RoomRenderer {
public:
    bool initialize(AAssetManager* assets, const RoomSettings& settings);
    void release();
    void setSettings(const RoomSettings& settings);
    void updateLocomotion(float leftX, float leftY, float rightX, float dt,
                          const XrPosef& headPose);
    void cycleScreenDestination();
    void cycleEnvironment();
    void toggleSitting(const XrPosef& headPose);
    bool sitting() const { return sitting_; }
    void renderEye(int eyeIndex, int width, int height, const XrView& view,
                   GLuint videoTexture, bool hasVideo);
    float locomotionX() const { return moveX_; }
    float locomotionZ() const { return moveZ_; }

private:
    struct Vertex { float x, y, z, u, v; };
    GLuint compileShader(GLenum type, const char* source);
    GLuint loadRgbAsset(AAssetManager* assets, const char* filename);
    GLuint createProceduralEnvironment(int sceneIndex);
    void createGeometry();
    void chooseEnvironment();
    void drawBox(const float* projectionView, float x, float y, float z,
                 float sx, float sy, float sz, const std::array<float, 4>& color,
                 float yaw = 0.0f);
    void drawPlane(const float* projectionView, float x, float y, float z,
                   float sx, float sy, GLuint texture, const std::array<float, 4>& color,
                   float yaw = 0.0f);
    void drawEnvironmentWindow(const float* projectionView, float halfW, float floorY, float backZ,
                               GLuint environmentTexture, float windowW, float windowH);

    RoomSettings settings_{};
    GLuint program_ = 0;
    GLuint vignetteProgram_ = 0;
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLint locMvp_ = -1;
    GLint locColor_ = -1;
    GLint locTexture_ = -1;
    GLint locTextured_ = -1;
    GLint locUvScale_ = -1;
    GLuint cubeTexture_ = 0;
    GLuint planeTexture_ = 0;
    std::array<GLuint, 88> environmentTextures_{};
    int activeEnvironmentIndex_ = 0;
    GLuint videoFallbackTexture_ = 0;
    bool initialized_ = false;
    float moveX_ = 0.0f;
    float moveZ_ = 0.0f;
    float virtualYaw_ = 0.0f;
    float snapCooldown_ = 0.0f;
    bool previousTurnRight_ = false;
    bool previousTurnLeft_ = false;
    bool sitting_ = false;
};
