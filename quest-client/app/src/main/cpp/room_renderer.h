#pragma once

#include <cstdint>
#include <string>
#include <array>
#include <GLES3/gl3.h>
#include <android/asset_manager.h>
#include <openxr/openxr.h>
#include "glb_model.h"

struct RoomSettings {
    std::string environment = "skyline_penthouse";       // 208 scenes; see Studio Virtual Room settings
    std::string roomSize = "medium";           // small | medium | large
    std::string screenDestination = "monitor"; // monitor | tv | both
    int monitorCount = 1;                       // 1..3, replicated stream
    bool showTv = true;
    bool showCouch = false; // Legacy config field; the bedroom renderer no longer draws a couch.
    bool showChair = false; // Legacy config setting; no chair is rendered or collided with.
    bool roomControlsEnabled = true;
    bool sittingToggleEnabled = false; // Legacy setting retained for config compatibility.
    float sittingHeadDropM = 0.42f; // Legacy setting; seated mode is no longer active.
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
    void toggleMenu();
    bool menuVisible() const { return menuVisible_; }
    void handleMenuInput(float vertical, bool selectPressed);
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
                 float yaw = 0.0f, int material = 0);
    void drawRoundedBox(const float* projectionView, float x, float y, float z,
                        float sx, float sy, float sz, const std::array<float, 4>& color,
                        float yaw = 0.0f, int material = 0);
    void drawPillow(const float* projectionView, float x, float y, float z,
                    float sx, float sy, float sz, const std::array<float, 4>& color,
                    float yaw = 0.0f, float pitch = 0.0f, int material = 3);
    void drawCurtainPanel(const float* projectionView, float x, float y, float z,
                          float width, float height, const std::array<float, 4>& color);
    void drawCylinder(const float* projectionView, float x, float y, float z,
                      float diameter, float height, const std::array<float, 4>& color,
                      int material = 0, float pitch = 0.0f);
    void drawText(const float* projectionView, const char* text, float x, float y, float z,
                  float pixelSize, const std::array<float, 4>& color);
    void drawSphere(const float* projectionView, float x, float y, float z,
                    float sx, float sy, float sz, const std::array<float, 4>& color,
                    int material = 0);
    void drawPlane(const float* projectionView, float x, float y, float z,
                   float sx, float sy, GLuint texture, const std::array<float, 4>& color,
                   float yaw = 0.0f);
    void drawCurvedPanel(const float* projectionView, float x, float y, float z,
                         float width, float height, float curveDepth, GLuint texture,
                         const std::array<float, 4>& color, int segments = 24);
    void drawGroundShadow(const float* projectionView, float x, float y, float z,
                          float width, float depth, float opacity);
    void drawEnvironmentWindow(const float* projectionView, float halfW, float floorY, float backZ,
                               GLuint environmentTexture, float windowW, float windowH);

    RoomSettings settings_{};
    GlbModel deskModel_;
    GlbModel monitorModel_;
    GLuint program_ = 0;
    GLuint vignetteProgram_ = 0;
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint sphereVao_ = 0;
    GLuint sphereVbo_ = 0;
    GLuint roundedVao_ = 0;
    GLuint roundedVbo_ = 0;
    GLsizei roundedVertexCount_ = 0;
    GLuint pillowVao_ = 0;
    GLuint pillowVbo_ = 0;
    GLsizei pillowVertexCount_ = 0;
    GLuint curtainVao_ = 0;
    GLuint curtainVbo_ = 0;
    GLsizei curtainVertexCount_ = 0;
    GLuint cylinderVao_ = 0;
    GLuint cylinderVbo_ = 0;
    GLsizei cylinderVertexCount_ = 0;
    GLsizei sphereVertexCount_ = 0;
    GLint locMvp_ = -1;
    GLint locColor_ = -1;
    GLint locTexture_ = -1;
    GLint locTextured_ = -1;
    GLint locUvScale_ = -1;
    GLint locMaterial_ = -1;
    GLint locModel_ = -1;
    GLint locCameraPosition_ = -1;
    GLint locRounded_ = -1;
    GLint locBevel_ = -1;
    GLint locMaterialAtlas_ = -1;
    GLint locHasMaterialAtlas_ = -1;
    GLint locNormalAtlas_ = -1;
    GLint locHasNormalAtlas_ = -1;
    GLint locRoughnessAtlas_ = -1;
    GLint locHasRoughnessAtlas_ = -1;
    GLint locCeilingLightPosition_ = -1;
    GLint locCeilingLightColor_ = -1;
    GLint locCeilingLightIntensity_ = -1;
    GLint locPracticalLightPosition_ = -1;
    GLint locPracticalLightColor_ = -1;
    GLint locPracticalLightIntensity_ = -1;
    GLint locNeonBluePosition_ = -1;
    GLint locNeonBlueColor_ = -1;
    GLint locNeonBlueIntensity_ = -1;
    GLint locNeonVioletPosition_ = -1;
    GLint locNeonVioletColor_ = -1;
    GLint locNeonVioletIntensity_ = -1;
    GLint locNeonPinkPosition_ = -1;
    GLint locNeonPinkColor_ = -1;
    GLint locNeonPinkIntensity_ = -1;
    GLuint cubeTexture_ = 0;
    GLuint planeTexture_ = 0;
    std::array<GLuint, 208> environmentTextures_{};
    int activeEnvironmentIndex_ = 0;
    GLuint videoFallbackTexture_ = 0;
    GLuint groundShadowTexture_ = 0;
    GLuint materialAtlasTexture_ = 0;
    GLuint normalAtlasTexture_ = 0;
    GLuint roughnessAtlasTexture_ = 0;
    GLuint textVao_ = 0;
    GLuint textVbo_ = 0;
    GLuint curvedVao_ = 0;
    GLuint curvedVbo_ = 0;
    bool initialized_ = false;
    float moveX_ = 0.0f;
    float moveZ_ = 0.0f;
    float virtualYaw_ = 0.0f;
    float snapCooldown_ = 0.0f;
    bool previousTurnRight_ = false;
    bool previousTurnLeft_ = false;
    bool menuVisible_ = false;
    int menuSelection_ = 0;
    bool previousMenuNavigation_ = false;
};
