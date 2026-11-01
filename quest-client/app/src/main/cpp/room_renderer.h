#pragma once

#include <cstdint>
#include <string>
#include <array>
#include <GLES3/gl3.h>
#include <android/asset_manager.h>
#include <openxr/openxr.h>
#include "glb_model.h"
#include "city_world.h"
#include "qbx/protocol.hpp"

struct RoomSettings {
    std::string environment = "skyline_penthouse";       // 209 scenes, including the streamed explorable city; see Studio settings
    std::string roomSize = "medium";           // small | medium | large
    std::string screenDestination = "monitor"; // monitor | tv | both
    int monitorCount = 1;                       // 1..3, replicated stream
    bool showTv = true;
    bool showCouch = false; // Legacy config field; the bedroom renderer no longer draws a couch.
    bool showChair = true; // User-supplied white gaming chair beside the desk.
    bool roomControlsEnabled = true;
    bool roomMovementEnabled = true;
    bool sittingToggleEnabled = false; // Legacy setting retained for config compatibility.
    float sittingHeadDropM = 0.42f; // Legacy setting; seated mode is no longer active.
    bool smoothTurn = false;
    bool comfortVignette = true;
    float movementSpeed = 1.35f;
    float turnSpeed = 1.8f;
    float snapTurnDegrees = 30.0f;
    float chairRollSpeed = 1.15f;
    float chairSpinSpeed = 1.55f;
    float chairFriction = 2.8f;
    float floorOffsetY = -1.60f;
};

class RoomRenderer {
public:
    bool initialize(AAssetManager* assets, const RoomSettings& settings);
    void release();
    void setSettings(const RoomSettings& settings);
    void updateLocomotion(float leftX, float leftY, float rightX, float dt,
                          const XrPosef& headPose);
    void handleRoomInteraction(const XrPosef& controllerAimPose, const XrPosef& headPose, bool triggerPressed);
    void setTrackedControllerPoses(const std::array<XrPosef,2>& poses,
                                   const std::array<bool,2>& valid);
    bool consumeMousePayload(qbx::MousePayload& payload);
    void cycleScreenDestination();
    void cycleEnvironment();
    void toggleMenu();
    bool menuVisible() const { return menuVisible_; }
    void handleMenuInput(float horizontal, float vertical, float adjust, bool selectPressed);
    void setConnectionState(bool connected, const std::string& hostIp, bool streaming);
    bool consumeReconnectRequest();
    void renderEye(int eyeIndex, int width, int height, const XrView& view,
                   const XrPosef& headCenterPose, GLuint videoTexture, bool hasVideo);
    float locomotionX() const { return moveX_; }
    float locomotionZ() const { return moveZ_; }

private:
    struct Vertex { float x, y, z, u, v; };
    GLuint compileShader(GLenum type, const char* source);
    GLuint loadRgbAsset(AAssetManager* assets, const char* filename);
    GLuint loadPanoramaAsset(AAssetManager* assets, const char* filename);
    GLuint createProceduralEnvironment(int sceneIndex);
    void createGeometry();
    void chooseEnvironment();
    void applyEnvironment(const std::string& environment);
    void selectRoomScene(int sceneIndex);
    void updateMousePayload(std::uint16_t buttons);
    std::array<float,3> ledZoneColor(int zone, float position) const;
    std::array<float,3> ledZoneAverageColor(int zone) const;
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
                          float width, float height, const std::array<float, 4>& color,
                          float yaw = 0.0f);
    void drawCylinder(const float* projectionView, float x, float y, float z,
                      float diameter, float height, const std::array<float, 4>& color,
                      int material = 0, float pitch = 0.0f);
    void drawText(const float* projectionView, const char* text, float x, float y, float z,
                  float pixelSize, const std::array<float, 4>& color,
                  float curveCenterX = 0.0f, float curveWidth = 0.0f, float curveDepth = 0.0f,
                  float curveCenterY = 0.0f);
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
    void drawEnvironmentWindow(const float* projectionView, float wallX, float centerZ, float floorY,
                               GLuint fallbackTexture, float windowW, float windowH, float yaw);
    void drawPanoramaSkybox(const float* projectionView, float cameraX, float cameraY, float cameraZ,
                            GLuint panoramaTexture);

    struct LedZoneSettings {
        std::array<float,3> primary{{0.08f,0.34f,1.0f}};
        std::array<float,3> secondary{{0.50f,0.12f,1.0f}};
        bool gradient = false;
        bool enabled = true;
        float brightness = 0.68f;
    };
    RoomSettings settings_{};
    std::array<LedZoneSettings,7> ledZones_{};
    bool ledDefaultsSeeded_ = false;
    GlbModel bedModel_;
    GlbModel deskModel_;
    GlbModel monitorModel_;
    GlbModel tvModel_;
    GlbModel chairModel_;
    GlbModel mouseModel_;
    GlbModel leftControllerModel_;
    GlbModel rightControllerModel_;
    std::array<XrPosef,2> controllerPoses_{};
    std::array<bool,2> controllerPoseValid_{{false,false}};
    CityWorld cityWorld_;
    bool citySpawnPending_ = true;
    bool cityReturnValid_ = false;
    float roomReturnMoveX_ = 0.0f;
    float roomReturnMoveZ_ = 0.0f;
    float roomReturnYaw_ = 0.0f;
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
    GLint locRoomAmbientIntensity_ = -1;
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
    GLint locPracticalLight2Position_ = -1;
    GLint locPracticalLight2Color_ = -1;
    GLint locPracticalLight2Intensity_ = -1;
    GLint locHeadboardGlowPosition_ = -1;
    GLint locHeadboardGlowColor_ = -1;
    GLint locHeadboardGlowIntensity_ = -1;
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
    GLuint mainRoomPanoramaTexture_ = 0;
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
    float chairOffsetX_ = 0.0f;
    float chairOffsetZ_ = 0.0f;
    float chairYaw_ = 0.0f;
    float chairVelocityX_ = 0.0f;
    float chairVelocityZ_ = 0.0f;
    float chairAngularVelocity_ = 0.0f;
    float chairWheelRoll_ = 0.0f;
    bool previousTurnRight_ = false;
    bool previousTurnLeft_ = false;
    bool menuVisible_ = false;
    int menuPage_ = 0;
    int menuSelection_ = 0;
    int environmentDropdownIndex_ = 0;
    bool environmentDropdownOpen_ = false;
    bool lightingColorPickerOpen_ = false;
    int lightingZoneIndex_ = 0;
    int lightingColorTarget_ = 0;
    float lightingPickerHue_ = 0.58f;
    float lightingPickerSaturation_ = 0.82f;
    float lightingPickerValue_ = 1.0f;
    bool previousMenuNavigation_ = false;
    bool previousMenuHorizontal_ = false;
    bool previousMenuAdjust_ = false;
    bool previousMenuPointerTrigger_ = false;
    bool menuPointerValid_ = false;
    std::array<float,3> menuPointerOrigin_{{0.0f,0.0f,0.0f}};
    std::array<float,3> menuPointerHit_{{0.0f,0.0f,0.0f}};
    bool reconnectRequested_ = false;
    bool roomLightsOn_ = true;
    bool previousRoomTrigger_ = false;
    bool mouseDragging_ = false;
    bool mousePayloadPending_ = false;
    bool sittingActive_ = false;
    bool sitAnchorPending_ = false;
    float seatedBlend_ = 0.0f;
    float mouseOffsetX_ = 0.39f;
    float mouseOffsetZ_ = 0.28f;
    float mouseGrabOffsetX_ = 0.0f;
    float mouseGrabOffsetZ_ = 0.0f;
    std::uint16_t mousePointerX_ = 32768;
    std::uint16_t mousePointerY_ = 32768;
    std::uint16_t mousePointerButtons_ = 0;
    std::uint16_t lastQueuedMouseX_ = 0;
    std::uint16_t lastQueuedMouseY_ = 0;
    bool pcConnected_ = false;
    bool pcStreaming_ = false;
    std::string pcHostIp_ = "Not configured";
};
