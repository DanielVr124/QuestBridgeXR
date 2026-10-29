#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "fake_vr_bridge.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string_view>

namespace {
std::string trim(std::string s) {
    auto notSpace=[](unsigned char c){return !std::isspace(c);};
    s.erase(s.begin(),std::find_if(s.begin(),s.end(),notSpace));
    s.erase(std::find_if(s.rbegin(),s.rend(),notSpace).base(),s.end());
    return s;
}
std::string lower(std::string s) { std::transform(s.begin(),s.end(),s.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));}); return s; }
bool parseBool(const std::string& s, bool fallback) {
    const auto v=lower(trim(s)); if(v=="true"||v=="1"||v=="yes"||v=="on")return true;
    if(v=="false"||v=="0"||v=="no"||v=="off")return false; return fallback;
}
float parseFloat(const std::string& s,float fallback,float lo,float hi) {
    try { size_t used=0; float v=std::stof(s,&used); if(used==s.size()&&std::isfinite(v)&&v>=lo&&v<=hi)return v; } catch(...){} return fallback;
}
int parseInt(const std::string& s,int fallback,int lo,int hi) {
    try { size_t used=0; int v=std::stoi(s,&used); if(used==s.size()&&v>=lo&&v<=hi)return v; } catch(...){} return fallback;
}
std::string foregroundExeName() {
    HWND window=GetForegroundWindow(); if(!window)return {};
    DWORD pid=0;GetWindowThreadProcessId(window,&pid);if(!pid)return {};
    HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);if(!process)return {};
    wchar_t path[32768]{};DWORD length=static_cast<DWORD>(std::size(path));
    const BOOL ok=QueryFullProcessImageNameW(process,0,path,&length);CloseHandle(process);if(!ok)return {};
    std::wstring full(path,length);const auto slash=full.find_last_of(L"\\/");
    std::wstring name=slash==std::wstring::npos?full:full.substr(slash+1);
    if(name.empty())return {};
    const int needed=WideCharToMultiByte(CP_UTF8,0,name.data(),static_cast<int>(name.size()),nullptr,0,nullptr,nullptr);
    if(needed<=0)return {};
    std::string out(static_cast<size_t>(needed),'\0');
    WideCharToMultiByte(CP_UTF8,0,name.data(),static_cast<int>(name.size()),out.data(),needed,nullptr,nullptr);
    return lower(out);
}
}

bool FakeVrBridge::load(const std::string& path) {
    std::ifstream input(path);
    if(!input.good()) {
        std::cout << "Fake-VR config not found at '" << path << "'; bridge disabled until Studio saves settings.\n";
        settings_.enabled=false;return false;
    }
    std::string line;
    while(std::getline(input,line)) {
        line=trim(line);if(line.empty()||line[0]=='#'||line[0]==';')continue;
        const auto equal=line.find('=');if(equal==std::string::npos)continue;
        const auto key=trim(line.substr(0,equal));const auto value=trim(line.substr(equal+1));
        if(key=="enabled")settings_.enabled=parseBool(value,settings_.enabled);
        else if(key=="allowed_processes")settings_.allowedProcesses=value;
        else if(key=="yaw_enabled")settings_.yawEnabled=parseBool(value,settings_.yawEnabled);
        else if(key=="pitch_enabled")settings_.pitchEnabled=parseBool(value,settings_.pitchEnabled);
        else if(key=="invert_pitch")settings_.invertPitch=parseBool(value,settings_.invertPitch);
        else if(key=="sensitivity_px_per_radian")settings_.sensitivityPxPerRadian=parseFloat(value,settings_.sensitivityPxPerRadian,50.0f,5000.0f);
        else if(key=="pitch_sensitivity")settings_.pitchSensitivity=parseFloat(value,settings_.pitchSensitivity,0.1f,3.0f);
        else if(key=="deadzone_radians")settings_.deadzoneRadians=parseFloat(value,settings_.deadzoneRadians,0.0f,0.1f);
        else if(key=="max_delta_pixels")settings_.maxDeltaPixels=parseInt(value,settings_.maxDeltaPixels,1,250);
    }
    allowedProcessNames_.clear();std::stringstream list(settings_.allowedProcesses);std::string item;
    while(std::getline(list,item,',')){item=lower(trim(item));if(!item.empty())allowedProcessNames_.push_back(item);}
    if(allowedProcessNames_.empty()){settings_.enabled=false;std::cerr<<"Fake-VR bridge disabled: process allowlist is empty.\n";return false;}
    std::cout<<"Fake-VR head-look bridge: "<<(settings_.enabled?"available (off until F8)":"disabled in config")
             <<"; allowed foreground process: "<<settings_.allowedProcesses<<"\n";
    return true;
}

float FakeVrBridge::wrapAngle(float a) {
    constexpr float pi=3.14159265358979323846f;
    while(a>pi)a-=2*pi;while(a < -pi)a+=2*pi;return a;
}
void FakeVrBridge::orientationToYawPitch(const float q[4],float& yaw,float& pitch) {
    // Quaternion wire order is x,y,z,w.
    const float x=q[0],y=q[1],z=q[2],w=q[3];
    yaw=std::atan2(2.0f*(w*y+x*z),1.0f-2.0f*(y*y+x*x));
    const float s=std::clamp(2.0f*(w*x-z*y),-1.0f,1.0f);
    pitch=std::asin(s);
}

void FakeVrBridge::pollHotkeys() {
    const bool f8=(GetAsyncKeyState(VK_F8)&0x8000)!=0;
    const bool f9=(GetAsyncKeyState(VK_F9)&0x8000)!=0;
    const bool esc=(GetAsyncKeyState(VK_ESCAPE)&0x8000)!=0;
    if(f8&&!previousF8Down_) {
        if(!settings_.enabled) std::cout<<"Fake-VR is disabled. Enable it on Studio's Fake VR / Game Bridge tab and restart the host.\n";
        else {active_=!active_;resetReference();std::cout<<"Fake-VR head-look "<<(active_?"ON":"paused")<<". Focus an allowlisted game window; ESC pauses.\n";}
    }
    if(f9&&!previousF9Down_){resetReference();std::cout<<"Fake-VR head-look reference recentered.\n";}
    if(esc&&!previousEscDown_&&active_){active_=false;resetReference();std::cout<<"Fake-VR head-look paused by ESC.\n";}
    previousF8Down_=f8;previousF9Down_=f9;previousEscDown_=esc;
}

bool FakeVrBridge::foregroundProcessAllowed() const {
    const std::string exe=foregroundExeName();
    return std::find(allowedProcessNames_.begin(),allowedProcessNames_.end(),exe)!=allowedProcessNames_.end();
}

void FakeVrBridge::onTracking(const qbx::TrackingPayload& tracking) {
    if(!settings_.enabled||!active_)return;
    if((tracking.trackingFlags&2u)==0u){resetReference();return;}
    if(!foregroundProcessAllowed()){resetReference();return;}
    float yaw=0,pitch=0;orientationToYawPitch(tracking.headOrientation,yaw,pitch);
    if(!haveReference_){previousYaw_=yaw;previousPitch_=pitch;haveReference_=true;return;}
    float dyaw=wrapAngle(yaw-previousYaw_);float dpitch=pitch-previousPitch_;previousYaw_=yaw;previousPitch_=pitch;
    if(std::fabs(dyaw)<settings_.deadzoneRadians)dyaw=0.0f;
    if(std::fabs(dpitch)<settings_.deadzoneRadians)dpitch=0.0f;
    int dx=settings_.yawEnabled?static_cast<int>(std::lround(dyaw*settings_.sensitivityPxPerRadian)):0;
    float pitchDirection=settings_.invertPitch?1.0f:-1.0f;
    int dy=settings_.pitchEnabled?static_cast<int>(std::lround(dpitch*settings_.sensitivityPxPerRadian*settings_.pitchSensitivity*pitchDirection)):0;
    dx=std::clamp(dx,-settings_.maxDeltaPixels,settings_.maxDeltaPixels);
    dy=std::clamp(dy,-settings_.maxDeltaPixels,settings_.maxDeltaPixels);
    if(dx==0&&dy==0)return;
    INPUT input{};input.type=INPUT_MOUSE;input.mi.dx=dx;input.mi.dy=dy;input.mi.dwFlags=MOUSEEVENTF_MOVE;
    // Standard OS input only: no process memory access, DLL injection, or game modification.
    SendInput(1,&input,sizeof(input));
}
