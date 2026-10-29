#include "room_renderer.h"

#include <android/log.h>
#include <android/asset_manager.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>

#define QBX_LOG_TAG "QuestBridgeRoom"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, QBX_LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, QBX_LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, QBX_LOG_TAG, __VA_ARGS__)

namespace {
constexpr float kPi = 3.14159265358979323846f;
const char* kVertexShader = R"GLSL(#version 300 es
precision highp float;
layout(location=0) in vec3 aPosition;
layout(location=1) in vec2 aUv;
uniform mat4 uMvp;
out vec2 vUv;
void main(){ gl_Position = uMvp * vec4(aPosition,1.0); vUv=aUv; }
)GLSL";
const char* kVignetteVertex = R"GLSL(#version 300 es
precision highp float;
layout(location=0) in vec3 aPosition;
layout(location=1) in vec2 aUv;
out vec2 vUv;
void main(){ gl_Position=vec4(aPosition.xy*2.0,0.0,1.0); vUv=aUv; }
)GLSL";
const char* kVignetteFragment = R"GLSL(#version 300 es
precision highp float;
in vec2 vUv;
out vec4 outColor;
void main(){ vec2 p=(vUv-vec2(0.5))*vec2(1.5,1.0); float a=smoothstep(0.22,0.82,length(p))*0.27; outColor=vec4(0.0,0.0,0.0,a); }
)GLSL";
const char* kFragmentShader = R"GLSL(#version 300 es
precision highp float;
in vec2 vUv;
uniform vec4 uColor;
uniform sampler2D uTexture;
uniform int uTextured;
out vec4 outColor;
void main(){
  vec4 tex = uTextured != 0 ? texture(uTexture, vUv) : vec4(1.0);
  outColor = tex * uColor;
}
)GLSL";

using Mat4 = std::array<float, 16>;
Mat4 identity() { return {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1}; }
Mat4 mul(const Mat4& a, const Mat4& b) {
    Mat4 out{};
    for (int c=0;c<4;++c) for (int r=0;r<4;++r)
        for (int k=0;k<4;++k) out[c*4+r] += a[k*4+r]*b[c*4+k];
    return out;
}
Mat4 translation(float x,float y,float z) { auto m=identity(); m[12]=x; m[13]=y; m[14]=z; return m; }
Mat4 scaling(float x,float y,float z) { auto m=identity(); m[0]=x; m[5]=y; m[10]=z; return m; }
Mat4 rotateY(float a) { auto m=identity(); const float c=std::cos(a),s=std::sin(a); m[0]=c; m[2]=-s; m[8]=s; m[10]=c; return m; }
Mat4 quatMatrix(XrQuaternionf q) {
    const float x=q.x,y=q.y,z=q.z,w=q.w;
    Mat4 m=identity();
    m[0]=1-2*(y*y+z*z); m[1]=2*(x*y+z*w); m[2]=2*(x*z-y*w);
    m[4]=2*(x*y-z*w); m[5]=1-2*(x*x+z*z); m[6]=2*(y*z+x*w);
    m[8]=2*(x*z+y*w); m[9]=2*(y*z-x*w); m[10]=1-2*(x*x+y*y);
    return m;
}
XrQuaternionf quatMul(const XrQuaternionf& a,const XrQuaternionf& b) {
    return {a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,
            a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,
            a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w,
            a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z};
}
Mat4 makeView(const XrPosef& pose, float locomotionX, float locomotionZ, float virtualYaw, float sittingDrop) {
    XrQuaternionf yawQ{0.0f, std::sin(virtualYaw*0.5f), 0.0f, std::cos(virtualYaw*0.5f)};
    XrQuaternionf q=quatMul(yawQ, pose.orientation);
    XrQuaternionf inv{-q.x,-q.y,-q.z,q.w};
    Mat4 r=quatMatrix(inv);
    Mat4 t=translation(-(pose.position.x+locomotionX),-(pose.position.y-sittingDrop),-(pose.position.z+locomotionZ));
    return mul(r,t);
}
Mat4 makeProjection(const XrFovf& fov) {
    const float l=std::tan(fov.angleLeft), r=std::tan(fov.angleRight);
    const float d=std::tan(fov.angleDown), u=std::tan(fov.angleUp);
    constexpr float n=0.05f, f=100.0f;
    Mat4 p{};
    p[0]=2.0f/(r-l); p[5]=2.0f/(u-d);
    p[8]=(r+l)/(r-l); p[9]=(u+d)/(u-d);
    p[10]=-(f+n)/(f-n); p[11]=-1.0f; p[14]=-(2.0f*f*n)/(f-n);
    return p;
}
float headYaw(const XrPosef& pose) {
    const auto q=pose.orientation;
    return std::atan2(2.0f*(q.w*q.y + q.x*q.z), 1.0f - 2.0f*(q.y*q.y + q.x*q.x));
}
std::array<float,4> rgb(float r,float g,float b,float a=1.0f) { return {r,g,b,a}; }
}

GLuint RoomRenderer::compileShader(GLenum type, const char* source) {
    GLuint shader=glCreateShader(type); glShaderSource(shader,1,&source,nullptr); glCompileShader(shader);
    GLint ok=GL_FALSE; glGetShaderiv(shader,GL_COMPILE_STATUS,&ok);
    if(ok!=GL_TRUE){ char log[1024]{}; glGetShaderInfoLog(shader,sizeof(log),nullptr,log); LOGE("room shader compile: %s",log); glDeleteShader(shader); return 0; }
    return shader;
}

GLuint RoomRenderer::loadRgbAsset(AAssetManager* assets, const char* filename) {
    if(!assets) return 0;
    AAsset* a=AAssetManager_open(assets,filename,AASSET_MODE_BUFFER);
    if(!a) { LOGW("environment asset missing: %s",filename); return 0; }
    const auto length=AAsset_getLength(a);
    std::vector<uint8_t> bytes(static_cast<size_t>(length));
    const int64_t read=AAsset_read(a,bytes.data(),bytes.size()); AAsset_close(a);
    if(read<8) return 0;
    uint32_t w=0,h=0; std::memcpy(&w,bytes.data(),4); std::memcpy(&h,bytes.data()+4,4);
    if(w<2||h<2||w>2048||h>2048||static_cast<uint64_t>(w)*h*3+8>static_cast<uint64_t>(read)) { LOGW("bad RGB asset: %s",filename); return 0; }
    GLuint t=0; glGenTextures(1,&t); glBindTexture(GL_TEXTURE_2D,t);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,static_cast<GLsizei>(w),static_cast<GLsizei>(h),0,GL_RGB,GL_UNSIGNED_BYTE,bytes.data()+8);
    glBindTexture(GL_TEXTURE_2D,0); LOGI("loaded %s (%ux%u)",filename,w,h); return t;
}

bool RoomRenderer::initialize(AAssetManager* assets,const RoomSettings& settings) {
    if(initialized_) return true;
    settings_=settings;
    GLuint vs=compileShader(GL_VERTEX_SHADER,kVertexShader), fs=compileShader(GL_FRAGMENT_SHADER,kFragmentShader);
    if(!vs||!fs){if(vs)glDeleteShader(vs);if(fs)glDeleteShader(fs);return false;}
    program_=glCreateProgram(); glAttachShader(program_,vs); glAttachShader(program_,fs); glLinkProgram(program_); glDeleteShader(vs); glDeleteShader(fs);
    GLint linked=GL_FALSE; glGetProgramiv(program_,GL_LINK_STATUS,&linked);
    if(linked!=GL_TRUE){char log[1024]{};glGetProgramInfoLog(program_,sizeof(log),nullptr,log);LOGE("room link: %s",log);glDeleteProgram(program_);program_=0;return false;}
    locMvp_=glGetUniformLocation(program_,"uMvp");locColor_=glGetUniformLocation(program_,"uColor");
    locTexture_=glGetUniformLocation(program_,"uTexture");locTextured_=glGetUniformLocation(program_,"uTextured");
    GLuint vvs=compileShader(GL_VERTEX_SHADER,kVignetteVertex), vfs=compileShader(GL_FRAGMENT_SHADER,kVignetteFragment);
    if(vvs&&vfs){vignetteProgram_=glCreateProgram();glAttachShader(vignetteProgram_,vvs);glAttachShader(vignetteProgram_,vfs);glLinkProgram(vignetteProgram_);GLint vok=GL_FALSE;glGetProgramiv(vignetteProgram_,GL_LINK_STATUS,&vok);if(vok!=GL_TRUE){glDeleteProgram(vignetteProgram_);vignetteProgram_=0;}}
    if(vvs)glDeleteShader(vvs);if(vfs)glDeleteShader(vfs);
    createGeometry();
    environmentTextures_[0]=loadRgbAsset(assets,"environments/blossom.rgb");
    environmentTextures_[1]=loadRgbAsset(assets,"environments/night_city.rgb");
    environmentTextures_[2]=loadRgbAsset(assets,"environments/golden_city.rgb");
    // A harmless fallback texture gives an intentional dark idle screen before a PC stream starts.
    const uint8_t idlePixels[4*4*3] = {
      18,24,39, 20,28,45, 20,28,45, 18,24,39, 20,28,45, 35,42,63, 35,42,63, 20,28,45,
      20,28,45, 35,42,63, 46,50,74, 20,28,45, 20,28,45, 35,42,63, 35,42,63, 20,28,45};
    glGenTextures(1,&videoFallbackTexture_);glBindTexture(GL_TEXTURE_2D,videoFallbackTexture_);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,4,4,0,GL_RGB,GL_UNSIGNED_BYTE,idlePixels);glBindTexture(GL_TEXTURE_2D,0);
    initialized_=true; chooseEnvironment(); LOGI("cozy room renderer initialized"); return true;
}

void RoomRenderer::createGeometry() {
    // Unit cube with UVs, 36 triangle vertices. The shader is intentionally unlit for a gentle stylized room.
    const float raw[] = {
      // front (+z)
      -0.5f,-0.5f,0.5f,0,0, 0.5f,-0.5f,0.5f,1,0, 0.5f,0.5f,0.5f,1,1,
      -0.5f,-0.5f,0.5f,0,0, 0.5f,0.5f,0.5f,1,1, -0.5f,0.5f,0.5f,0,1,
      // back (-z)
      0.5f,-0.5f,-0.5f,0,0, -0.5f,-0.5f,-0.5f,1,0, -0.5f,0.5f,-0.5f,1,1,
      0.5f,-0.5f,-0.5f,0,0, -0.5f,0.5f,-0.5f,1,1, 0.5f,0.5f,-0.5f,0,1,
      // right
      0.5f,-0.5f,0.5f,0,0, 0.5f,-0.5f,-0.5f,1,0, 0.5f,0.5f,-0.5f,1,1,
      0.5f,-0.5f,0.5f,0,0, 0.5f,0.5f,-0.5f,1,1, 0.5f,0.5f,0.5f,0,1,
      // left
      -0.5f,-0.5f,-0.5f,0,0, -0.5f,-0.5f,0.5f,1,0, -0.5f,0.5f,0.5f,1,1,
      -0.5f,-0.5f,-0.5f,0,0, -0.5f,0.5f,0.5f,1,1, -0.5f,0.5f,-0.5f,0,1,
      // top
      -0.5f,0.5f,0.5f,0,0, 0.5f,0.5f,0.5f,1,0, 0.5f,0.5f,-0.5f,1,1,
      -0.5f,0.5f,0.5f,0,0, 0.5f,0.5f,-0.5f,1,1, -0.5f,0.5f,-0.5f,0,1,
      // bottom
      -0.5f,-0.5f,-0.5f,0,0, 0.5f,-0.5f,-0.5f,1,0, 0.5f,-0.5f,0.5f,1,1,
      -0.5f,-0.5f,-0.5f,0,0, 0.5f,-0.5f,0.5f,1,1, -0.5f,-0.5f,0.5f,0,1
    };
    glGenVertexArrays(1,&vao_);glGenBuffers(1,&vbo_);glBindVertexArray(vao_);glBindBuffer(GL_ARRAY_BUFFER,vbo_);
    glBufferData(GL_ARRAY_BUFFER,sizeof(raw),raw,GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,5*sizeof(float),reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,5*sizeof(float),reinterpret_cast<void*>(3*sizeof(float)));
    glBindVertexArray(0);
    const float plane[] = {-0.5f,-0.5f,0,0,1, 0.5f,-0.5f,0,1,1, 0.5f,0.5f,0,1,0, -0.5f,-0.5f,0,0,1, 0.5f,0.5f,0,1,0, -0.5f,0.5f,0,0,0};
    glGenVertexArrays(1,&planeTexture_); // plane VAO handles geometry separately
    glGenBuffers(1,&cubeTexture_); // IDs are reused as plane VAO/VBO handles; no texture objects here
    glBindVertexArray(planeTexture_);glBindBuffer(GL_ARRAY_BUFFER,cubeTexture_);glBufferData(GL_ARRAY_BUFFER,sizeof(plane),plane,GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,5*sizeof(float),reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,5*sizeof(float),reinterpret_cast<void*>(3*sizeof(float)));
    glBindVertexArray(0);
}

void RoomRenderer::setSettings(const RoomSettings& settings) { settings_=settings; chooseEnvironment(); }
void RoomRenderer::chooseEnvironment() {
    activeEnvironmentIndex_=settings_.environment=="night_city"?1:settings_.environment=="golden_city"?2:0;
}
void RoomRenderer::cycleScreenDestination() {
    if (!settings_.roomControlsEnabled) return;
    if (settings_.screenDestination == "monitor") settings_.screenDestination = "tv";
    else if (settings_.screenDestination == "tv") settings_.screenDestination = "both";
    else settings_.screenDestination = "monitor";
    LOGI("Room screen destination changed to %s", settings_.screenDestination.c_str());
}
void RoomRenderer::cycleEnvironment() {
    if (!settings_.roomControlsEnabled) return;
    if (activeEnvironmentIndex_ == 0) settings_.environment = "night_city";
    else if (activeEnvironmentIndex_ == 1) settings_.environment = "golden_city";
    else settings_.environment = "blossom";
    chooseEnvironment();
    LOGI("Room scenery changed to %s", settings_.environment.c_str());
}
void RoomRenderer::toggleSitting(const XrPosef& headPose) {
    if (!settings_.roomControlsEnabled || !settings_.sittingToggleEnabled) return;
    if (sitting_) {
        sitting_ = false;
        LOGI("Stood up from virtual seat");
        return;
    }

    float halfW=3.0f, halfD=3.8f;
    if(settings_.roomSize=="large"){halfW=6.0f;halfD=7.5f;}
    else if(settings_.roomSize=="medium"){halfW=4.2f;halfD=5.4f;}
    const float playerX=headPose.position.x+moveX_;
    const float playerZ=headPose.position.z+moveZ_;
    struct Seat { float x,z,yaw; const char* name; bool enabled; };
    const Seat seats[] = {
        {0.85f, 1.6f, 0.0f, "desk chair", settings_.showChair},
        {-halfW+1.25f, -halfD+0.80f, kPi, "couch", settings_.showCouch}
    };
    const Seat* nearest=nullptr;
    float nearestDistance=1.10f;
    for (const auto& seat : seats) {
        if (!seat.enabled) continue;
        const float dx=playerX-seat.x, dz=playerZ-seat.z;
        const float distance=std::sqrt(dx*dx+dz*dz);
        if(distance<nearestDistance){nearestDistance=distance;nearest=&seat;}
    }
    if(!nearest){LOGI("Move close to the desk chair or couch before sitting");return;}

    // Rebase the virtual room so the current tracked head position sits at the seat anchor.
    // Real headset tracking remains untouched; this only positions the virtual avatar/camera.
    moveX_=nearest->x-headPose.position.x;
    moveZ_=nearest->z-headPose.position.z;
    virtualYaw_=nearest->yaw-headYaw(headPose);
    sitting_=true;
    LOGI("Sat on the virtual %s",nearest->name);
}

void RoomRenderer::drawBox(const float* pv,float x,float y,float z,float sx,float sy,float sz,const std::array<float,4>& color,float yaw) {
    Mat4 projectionView{}; std::copy(pv, pv + projectionView.size(), projectionView.begin());
    Mat4 model=mul(translation(x,y,z),mul(rotateY(yaw),scaling(sx,sy,sz))); Mat4 mvp=mul(projectionView,model);
    glUniformMatrix4fv(locMvp_,1,GL_FALSE,mvp.data());glUniform4fv(locColor_,1,color.data());glUniform1i(locTextured_,0);
    glBindVertexArray(vao_);glDrawArrays(GL_TRIANGLES,0,36);glBindVertexArray(0);
}
void RoomRenderer::drawPlane(const float* pv,float x,float y,float z,float sx,float sy,GLuint texture,const std::array<float,4>& color,float yaw) {
    Mat4 projectionView{}; std::copy(pv, pv + projectionView.size(), projectionView.begin());
    Mat4 model=mul(translation(x,y,z),mul(rotateY(yaw),scaling(sx,sy,1)));Mat4 mvp=mul(projectionView,model);
    glUniformMatrix4fv(locMvp_,1,GL_FALSE,mvp.data());glUniform4fv(locColor_,1,color.data());glUniform1i(locTextured_,texture?1:0);
    glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,texture?texture:videoFallbackTexture_);glUniform1i(locTexture_,0);
    glBindVertexArray(planeTexture_);glDrawArrays(GL_TRIANGLES,0,6);glBindVertexArray(0);glBindTexture(GL_TEXTURE_2D,0);
}

void RoomRenderer::drawEnvironmentWindow(const float* pv,float halfW,float floorY,float backZ,GLuint tex,float windowW,float windowH) {
    (void)halfW;
    const float windowY=floorY+2.4f;
    // A broad window with warm wooden frame rails. The photo becomes the outside view; all nearby surfaces are real 3D geometry.
    drawPlane(pv,0,windowY,backZ+0.035f,windowW,windowH,tex,rgb(1,1,1));
    const auto frame=rgb(0.20f,0.14f,0.13f);
    const float t=0.075f;
    drawBox(pv,0,windowY+windowH*.5f+t*.5f,backZ+0.015f,windowW+2*t,t,0.12f,frame);
    drawBox(pv,0,windowY-windowH*.5f-t*.5f,backZ+0.015f,windowW+2*t,t,0.12f,frame);
    drawBox(pv,-windowW*.5f-t*.5f,windowY,backZ+0.015f,t,windowH,0.12f,frame);
    drawBox(pv,windowW*.5f+t*.5f,windowY,backZ+0.015f,t,windowH,0.12f,frame);
    drawBox(pv,0,windowY,backZ+0.005f,0.035f,windowH,0.10f,frame);
}

void RoomRenderer::updateLocomotion(float leftX,float leftY,float rightStickX,float dt,const XrPosef& headPose) {
    if(dt<0.0f||dt>0.1f)dt=0.0f;
    // Pushing the left stick stands up automatically; the virtual seat is not a locked animation state.
    if(sitting_ && (std::fabs(leftX)>0.22f || std::fabs(leftY)>0.22f)) sitting_=false;
    const float yaw=headYaw(headPose)+virtualYaw_;
    const float speed=std::clamp(settings_.movementSpeed,0.1f,4.0f);
    // Left stick uses head-relative direction; reverse sign because forward is -Z in OpenXR.
    const float forwardX=std::sin(yaw), forwardZ=-std::cos(yaw);
    const float rightX=std::cos(yaw), rightZ=std::sin(yaw);
    const float oldMoveX = moveX_, oldMoveZ = moveZ_;
    moveX_+=(rightX*leftX+forwardX*leftY)*speed*dt;
    moveZ_+=(rightZ*leftX+forwardZ*leftY)*speed*dt;
    float halfW=3.0f,halfD=3.8f;
    if(settings_.roomSize=="large"){halfW=6.0f;halfD=7.5f;}else if(settings_.roomSize=="medium"){halfW=4.2f;halfD=5.4f;}
    moveX_=std::clamp(moveX_,-halfW+0.55f,halfW-0.55f);
    moveZ_=std::clamp(moveZ_,-halfD+0.55f,halfD-0.55f);
    auto overlaps = [](float x, float z, float cx, float cz, float rx, float rz) {
        return std::fabs(x-cx) < rx && std::fabs(z-cz) < rz;
    };
    // Simple non-physics collision boxes keep joystick movement from passing through furniture.
    const float couchX = -halfW + 1.25f, couchZ = -halfD + 0.80f;
    const bool hitsDesk = overlaps(moveX_, moveZ_, 0.0f, -1.75f, 1.72f, 0.68f);
    const bool hitsChair = settings_.showChair && overlaps(moveX_, moveZ_, 0.85f, 1.6f, 0.55f, 0.54f);
    const bool hitsCouch = settings_.showCouch && overlaps(moveX_, moveZ_, couchX, couchZ, 1.18f, 0.65f);
    if (hitsDesk || hitsChair || hitsCouch) { moveX_ = oldMoveX; moveZ_ = oldMoveZ; }
    snapCooldown_=std::max(0.0f,snapCooldown_-dt);
    if(settings_.smoothTurn){virtualYaw_+=std::clamp(rightStickX,-1.0f,1.0f)*std::clamp(settings_.turnSpeed,0.2f,3.5f)*dt;}
    else {
        const bool right=rightStickX>0.7f,left=rightStickX < -0.7f;
        if(snapCooldown_<=0.0f && right && !previousTurnRight_){virtualYaw_-=settings_.snapTurnDegrees*kPi/180.0f;snapCooldown_=0.24f;}
        else if(snapCooldown_<=0.0f && left && !previousTurnLeft_){virtualYaw_+=settings_.snapTurnDegrees*kPi/180.0f;snapCooldown_=0.24f;}
        previousTurnRight_=right;previousTurnLeft_=left;
    }
}

void RoomRenderer::renderEye(int eyeIndex,int width,int height,const XrView& view,GLuint videoTexture,bool hasVideo) {
    if(!initialized_)return;
    float halfW=3.0f,halfD=3.8f;
    if(settings_.roomSize=="large"){halfW=6.0f;halfD=7.5f;}else if(settings_.roomSize=="medium"){halfW=4.2f;halfD=5.4f;}
    (void)eyeIndex; (void)width; (void)height;
    const float floorY=settings_.floorOffsetY, ceilingY=floorY+4.4f, backZ=-halfD, frontZ=halfD;
    Mat4 projection=makeProjection(view.fov);
    const float sittingDrop = sitting_ ? std::clamp(settings_.sittingHeadDropM, 0.10f, 0.75f) : 0.0f;
    Mat4 camera=makeView(view.pose,moveX_,moveZ_,virtualYaw_,sittingDrop);
    Mat4 pv=mul(projection,camera);
    const auto& env=environmentTextures_[activeEnvironmentIndex_];
    std::array<float,4> wall=activeEnvironmentIndex_==0?rgb(.70f,.63f,.67f):activeEnvironmentIndex_==1?rgb(.10f,.13f,.20f):rgb(.20f,.15f,.12f);
    std::array<float,4> floor=activeEnvironmentIndex_==0?rgb(.44f,.37f,.41f):activeEnvironmentIndex_==1?rgb(.10f,.14f,.19f):rgb(.17f,.12f,.09f);
    std::array<float,4> wood=activeEnvironmentIndex_==0?rgb(.32f,.20f,.21f):activeEnvironmentIndex_==1?rgb(.12f,.14f,.17f):rgb(.30f,.18f,.09f);
    std::array<float,4> sofa=activeEnvironmentIndex_==0?rgb(.63f,.38f,.48f):activeEnvironmentIndex_==1?rgb(.11f,.20f,.28f):rgb(.45f,.25f,.09f);
    glUseProgram(program_);glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);glDisable(GL_CULL_FACE);
    glUniform1i(locTexture_,0);
    // Room shell: floor, ceiling, side walls and back wall. Shapes stay anchored in world coordinates.
    drawBox(pv.data(),0,floorY-0.08f,0,halfW*2+0.1f,0.16f,halfD*2+0.1f,floor);
    drawBox(pv.data(),0,ceilingY+0.05f,0,halfW*2+0.1f,0.1f,halfD*2+0.1f,wall);
    drawBox(pv.data(),-halfW-0.05f,(floorY+ceilingY)*.5f,0,0.1f,ceilingY-floorY,halfD*2,wall);
    drawBox(pv.data(),halfW+0.05f,(floorY+ceilingY)*.5f,0,0.1f,ceilingY-floorY,halfD*2,wall);
    drawBox(pv.data(),0,(floorY+ceilingY)*.5f,backZ-0.05f,halfW*2,ceilingY-floorY,0.1f,wall);
    // Front wall has a centered doorway, so looking around still feels like being inside a room.
    const float sidePanelWidth = std::max(0.25f, (halfW * 2.0f - 1.4f) * 0.5f);
    drawBox(pv.data(),-(halfW+0.7f)*0.5f,(floorY+ceilingY)*0.5f,frontZ+0.02f,
            sidePanelWidth,ceilingY-floorY,0.10f,wall);
    drawBox(pv.data(), (halfW+0.7f)*0.5f,(floorY+ceilingY)*0.5f,frontZ+0.02f,
            sidePanelWidth,ceilingY-floorY,0.10f,wall);
    drawBox(pv.data(),0,ceilingY-1.05f,frontZ+0.02f,1.4f,2.1f,0.10f,wall);
    drawEnvironmentWindow(pv.data(),halfW,floorY,backZ,env,std::min(halfW*1.35f,settings_.roomSize=="small"?3.4f:6.2f),activeEnvironmentIndex_==1?2.55f:1.8f);
    // Desk and monitor mount.
    const float deskZ=-1.75f;
    drawBox(pv.data(),0,floorY+0.78f,deskZ,3.25f,0.10f,0.82f,wood);
    drawBox(pv.data(),-1.38f,floorY+0.37f,deskZ,0.10f,0.75f,0.72f,wood);
    drawBox(pv.data(),1.38f,floorY+0.37f,deskZ,0.10f,0.75f,0.72f,wood);
    const int monitorCount=std::clamp(settings_.monitorCount,1,3);
    const float monitorW=monitorCount==1?1.28f:monitorCount==2?0.90f:0.72f;
    const float monitorH=monitorW*9.0f/16.0f;
    const float monitorCenterY=floorY+1.35f;
    const float deskTopY=floorY+0.83f;
    const float standHeight=std::max(0.08f,monitorCenterY-monitorH*0.5f-deskTopY);
    const float gap=0.08f;
    for(int i=0;i<monitorCount;++i){
        const float x=(i-(monitorCount-1)*0.5f)*(monitorW+gap);
        const bool streamOn=(settings_.screenDestination=="monitor"||settings_.screenDestination=="both");
        GLuint tex=(streamOn&&hasVideo)?videoTexture:videoFallbackTexture_;
        drawBox(pv.data(),x,monitorCenterY,deskZ-0.05f,monitorW+0.05f,monitorH+0.05f,0.08f,rgb(.05f,.06f,.09f));
        drawPlane(pv.data(),x,monitorCenterY,deskZ+0.001f,monitorW,monitorH,tex,rgb(1,1,1));
        drawBox(pv.data(),x,deskTopY+standHeight*0.5f,deskZ,0.05f,standHeight,0.08f,wood);
    }
    // Optional wall-mounted television on the right wall; source can be switched between monitor and TV or mirrored.
    if(settings_.showTv){
        const bool tvStream=(settings_.screenDestination=="tv"||settings_.screenDestination=="both");
        GLuint tvTex=(tvStream&&hasVideo)?videoTexture:env;
        drawBox(pv.data(),halfW-0.12f,floorY+2.0f,-2.4f,0.10f,1.32f,2.28f,rgb(.035f,.04f,.055f),0);
        // The television screen faces toward room center (-X); rotate local +Z to -X.
        drawPlane(pv.data(),halfW-0.055f,floorY+2.0f,-2.4f,1.95f,1.08f,tvTex,rgb(1,1,1),-kPi*.5f);
    }
    // Sofa with separate seat/back/arms; large enough to read as a comfortable place to sit.
    if(settings_.showCouch){
        const float couchX=-halfW+1.25f,couchZ=-halfD+0.80f;
        drawBox(pv.data(),couchX,floorY+0.47f,couchZ,2.0f,0.35f,0.88f,sofa);
        drawBox(pv.data(),couchX,floorY+0.88f,couchZ-0.31f,2.0f,0.62f,0.22f,sofa);
        drawBox(pv.data(),couchX-0.91f,floorY+0.64f,couchZ,0.22f,0.48f,0.90f,sofa);
        drawBox(pv.data(),couchX+0.91f,floorY+0.64f,couchZ,0.22f,0.48f,0.90f,sofa);
        drawBox(pv.data(),couchX-0.48f,floorY+0.68f,couchZ+0.03f,0.78f,0.08f,0.70f,rgb(sofa[0]*1.12f,sofa[1]*1.12f,sofa[2]*1.12f));
        drawBox(pv.data(),couchX+0.48f,floorY+0.68f,couchZ+0.03f,0.78f,0.08f,0.70f,rgb(sofa[0]*1.12f,sofa[1]*1.12f,sofa[2]*1.12f));
    }
    // A rolling desk chair behind the spawn position to walk back to and sit in physically.
    if(settings_.showChair){
        const float chairX=0.85f, chairZ=1.6f;
        drawBox(pv.data(),chairX,floorY+0.58f,chairZ,0.72f,0.18f,0.66f,sofa);
        drawBox(pv.data(),chairX,floorY+1.05f,chairZ-0.25f,0.72f,0.82f,0.16f,sofa);
        drawBox(pv.data(),chairX,floorY+0.28f,chairZ,0.08f,0.52f,0.08f,wood);
        for(int i=0;i<5;++i){float a=2*kPi*i/5;drawBox(pv.data(),chairX+std::cos(a)*.30f,floorY+0.05f,chairZ+std::sin(a)*.26f,.34f,.05f,.06f,wood,a);}
    }
    // Side lamps add warm points of color without expensive lights or post processing.
    drawBox(pv.data(),-halfW+0.32f,floorY+1.45f,-1.7f,0.12f,1.0f,0.12f,wood);
    drawBox(pv.data(),-halfW+0.32f,floorY+2.02f,-1.7f,0.42f,0.28f,0.42f,activeEnvironmentIndex_==0?rgb(.98f,.72f,.82f):rgb(1.0f,.63f,.30f));
    if(settings_.comfortVignette && vignetteProgram_){
        glDisable(GL_DEPTH_TEST);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(vignetteProgram_);glBindVertexArray(planeTexture_);glDrawArrays(GL_TRIANGLES,0,6);glBindVertexArray(0);
        glUseProgram(0);glDisable(GL_BLEND);glEnable(GL_DEPTH_TEST);
    }
    glBindTexture(GL_TEXTURE_2D,0);glBindVertexArray(0);glUseProgram(0);glDisable(GL_DEPTH_TEST);
}

void RoomRenderer::release() {
    if(!initialized_&&!program_&&!vao_)return;
    if(program_)glDeleteProgram(program_);if(vignetteProgram_)glDeleteProgram(vignetteProgram_);if(vao_)glDeleteVertexArrays(1,&vao_);if(vbo_)glDeleteBuffers(1,&vbo_);
    if(planeTexture_)glDeleteVertexArrays(1,&planeTexture_);if(cubeTexture_)glDeleteBuffers(1,&cubeTexture_);
    for(auto t:environmentTextures_)if(t)glDeleteTextures(1,&t);if(videoFallbackTexture_)glDeleteTextures(1,&videoFallbackTexture_);
    program_=vignetteProgram_=vao_=vbo_=planeTexture_=cubeTexture_=videoFallbackTexture_=0;environmentTextures_.fill(0);initialized_=false;
}
