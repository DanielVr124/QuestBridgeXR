#include "room_renderer.h"

#include <android/log.h>
#include <android/asset_manager.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <fstream>
#include <iterator>
#include <vector>

#define QBX_LOG_TAG "QuestBridgeRoom"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, QBX_LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, QBX_LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, QBX_LOG_TAG, __VA_ARGS__)

namespace {
constexpr float kPi = 3.14159265358979323846f;
constexpr std::array<const char*, 208> kEnvironmentNames = {


    "blossom",
    "night_city",
    "golden_city",
    "aurora",
    "cyberpunk",
    "sunset_beach",
    "snowy_mountains",
    "enchanted_forest",
    "desert_dunes",
    "ocean_cliffs",
    "space_nebula",
    "lunar_base",
    "volcanic",
    "underwater",
    "autumn_forest",
    "tropical_island",
    "candy_land",
    "steampunk_city",
    "rainy_tokyo",
    "sakura_night",
    "alpine_lake",
    "emerald_valley",
    "wasteland",
    "midnight_clouds",
    "crystal_cavern",
    "lavender_fields",
    "firefly_swamp",
    "polar_station",
    "neon_downtown",
    "skyline_penthouse",
    "harbor_city",
    "rainy_metropolis",
    "old_town_evening",
    "rooftop_sunset",
    "future_megacity",
    "night_market",
    "redwood_forest",
    "bamboo_grove",
    "misty_woodland",
    "wildflower_meadow",
    "jungle_canopy",
    "waterfall_glen",
    "apple_orchard",
    "mossy_forest",
    "coral_reef",
    "turquoise_lagoon",
    "moonlit_ocean",
    "stormy_coast",
    "ice_fjord",
    "river_valley",
    "seaside_village",
    "bioluminescent_bay",
    "alpine_sunrise",
    "winter_pines",
    "glacier_valley",
    "highland_lake",
    "mountain_village",
    "snow_cabin",
    "aurora_ridge",
    "polar_twilight",
    "mars_frontier",
    "starship_bridge",
    "binary_stars",
    "alien_planet",
    "crystal_planet",
    "floating_islands",
    "fantasy_castle",
    "moon_garden",
    "desert_oasis",
    "red_canyon",
    "golden_badlands",
    "volcanic_ash",
    "lava_cavern",
    "salt_flats",
    "misty_moor",
    "thunderhead",
    "spring_garden",
    "summer_meadow",
    "autumn_orchard",
    "winter_village",
    "lantern_festival",
    "sakura_festival",
    "lavender_twilight",
    "golden_hour_hills",
    "dreamscape",
    "vaporwave_dream",
    "synthwave_highway",
    "crystal_forest",
    "dreamcore_meadow",
    "dreamcore_cloud_room",
    "endless_hallway",
    "liminal_poolrooms",
    "empty_mall_after_hours",
    "pastel_playground",
    "quiet_school_hallway",
    "soft_dreamscape",
    "cozy_cabin_snowfall",
    "rainy_cafe_window",
    "fireplace_reading_nook",
    "warm_attic",
    "cottagecore_kitchen",
    "blanket_fort",
    "soft_lighting_bedroom",
    "quiet_bookshop",
    "cloud_sea_overlook",
    "quiet_lake_dock",
    "moonlit_garden",
    "misty_waterfall",
    "sunlit_forest_glade",
    "lavender_cottage",
    "meadow_stargazing",
    "rainy_japanese_garden",
    "fairy_village",
    "mushroom_forest",
    "floating_castle",
    "starlight_garden",
    "unicorn_valley",
    "enchanted_library",
    "fairy_lantern_path",
    "celestial_temple",
    "underwater_palace",
    "sky_island_city",
    "cloud_kingdom",
    "crystal_lake",
    "rainbow_waterfall",
    "moon_base_garden",
    "ancient_ruins",
    "celestial_ocean",
    "snow_globe_village",
    "autumn_cabin",
    "spring_rain_garden",
    "summer_lakeside",
    "christmas_market",
    "lantern_river",
    "harvest_festival",
    "snowy_pine_retreat",
    "gentle_rain_rooftop",
    "foggy_seaside",
    "thunderstorm_shelter",
    "dawn_mountains",
    "blue_hour_city",
    "golden_sunrise_forest",
    "quiet_snowfall",
    "cloud_shadow_valley",
    "kyoto_garden",
    "tokyo_rainy_alley",
    "japanese_onsen",
    "bamboo_temple",
    "seaside_ryokan",
    "cherry_blossom_river",
    "lantern_town",
    "mount_fuji_sunrise",
    "aquarium_dream",
    "coral_at_night",
    "jellyfish_lagoon",
    "tropical_shoreline",
    "sea_cave",
    "underwater_ruins",
    "moonlit_beach",
    "coastal_lighthouse",
    "ringed_planet",
    "nebula_clouds",
    "orbital_observatory",
    "mars_dusk",
    "alien_ocean",
    "moonlit_craters",
    "stellar_nursery",
    "wormhole_sky",
    "pastel_rainbow_hills",
    "candy_clouds",
    "toyland_sunset",
    "pastel_city",
    "bubble_garden",
    "cotton_candy_sky",
    "rainbow_valley",
    "fantasy_boardwalk",
    "night_rooftop_garden",
    "minimalist_penthouse",
    "skyscraper_sunrise",
    "quiet_cafe_street",
    "neon_arcade_district",
    "balcony_rain",
    "city_park_evening",
    "riverside_citywalk",
    "giant_moon_field",
    "floating_staircase",
    "infinite_mirror_lake",
    "hallway_to_clouds",
    "impossible_staircase_sky",
    "doorway_in_meadow",
    "upside_down_garden",
    "clockwork_dream",
    "meditation_garden",
    "zen_sand_garden",
    "secluded_hot_springs",
    "pine_forest_retreat",
    "quiet_riverbank",
    "mountain_morning_mist",
    "seaside_meditation",
    "candlelit_sanctuary",
    "obsidian_citadel",
    "moonstone_cavern",
    "glowing_spore_forest",
    "starfall_valley",
    "celestial_garden",
    "ancient_sky_ruins",
    "midnight_blossom_grove",
    "aurora_lagoon",
};


struct EnvironmentPalette {
    std::array<float, 3> skyTop;
    std::array<float, 3> skyHorizon;
    std::array<float, 3> ground;
    float horizon;
};

EnvironmentPalette extraEnvironmentPalette(int extraIndex) {
    EnvironmentPalette p{};
    // Keep the original groups and align each new environment pack at 8 scenes.
    const int paletteGroup = extraIndex < 60 ? extraIndex / 8 : 8 + (extraIndex - 60) / 8;
    switch (paletteGroup) {
    case 0: p.skyTop={10,13,39}; p.skyHorizon={70,69,136}; p.ground={8,18,39}; p.horizon=0.82f; break;
    case 1: p.skyTop={20,58,49}; p.skyHorizon={105,162,119}; p.ground={18,58,41}; p.horizon=0.70f; break;
    case 2: p.skyTop={8,54,76}; p.skyHorizon={75,175,190}; p.ground={5,75,95}; p.horizon=0.68f; break;
    case 3: p.skyTop={33,76,112}; p.skyHorizon={164,200,222}; p.ground={69,112,139}; p.horizon=0.65f; break;
    case 4: p.skyTop={13,10,49}; p.skyHorizon={78,56,132}; p.ground={22,18,64}; p.horizon=0.84f; break;
    case 5: p.skyTop={66,31,25}; p.skyHorizon={210,119,66}; p.ground={124,72,43}; p.horizon=0.69f; break;
    case 6: p.skyTop={73,68,119}; p.skyHorizon={205,157,166}; p.ground={101,93,120}; p.horizon=0.66f; break;
    case 8: p.skyTop={91,83,142}; p.skyHorizon={206,177,225}; p.ground={136,124,175}; p.horizon=0.72f; break;
    case 9: p.skyTop={53,49,71}; p.skyHorizon={198,142,115}; p.ground={84,67,73}; p.horizon=0.74f; break;
    case 10: p.skyTop={39,88,109}; p.skyHorizon={172,207,176}; p.ground={65,131,102}; p.horizon=0.64f; break;
    case 11: p.skyTop={28,17,62}; p.skyHorizon={132,102,185}; p.ground={55,61,106}; p.horizon=0.72f; break;
    case 12: p.skyTop={18,28,82}; p.skyHorizon={84,139,198}; p.ground={28,77,111}; p.horizon=0.68f; break;
    case 13: p.skyTop={99,80,105}; p.skyHorizon={235,170,139}; p.ground={112,97,87}; p.horizon=0.65f; break;
    case 14: p.skyTop={42,63,80}; p.skyHorizon={148,165,175}; p.ground={61,84,98}; p.horizon=0.72f; break;
    case 15: p.skyTop={19,43,75}; p.skyHorizon={159,144,170}; p.ground={67,92,102}; p.horizon=0.68f; break;
    case 16: p.skyTop={2,38,63}; p.skyHorizon={17,136,163}; p.ground={2,62,83}; p.horizon=0.72f; break;
    case 17: p.skyTop={3,3,28}; p.skyHorizon={48,25,90}; p.ground={12,14,41}; p.horizon=0.86f; break;
    case 18: p.skyTop={188,169,235}; p.skyHorizon={255,197,217}; p.ground={149,121,184}; p.horizon=0.65f; break;
    case 19: p.skyTop={18,26,53}; p.skyHorizon={87,106,152}; p.ground={23,34,56}; p.horizon=0.82f; break;
    case 20: p.skyTop={51,48,92}; p.skyHorizon={176,168,211}; p.ground={103,121,164}; p.horizon=0.75f; break;
    case 21: p.skyTop={46,79,78}; p.skyHorizon={176,194,166}; p.ground={84,119,96}; p.horizon=0.64f; break;
    case 22: p.skyTop={10,20,52}; p.skyHorizon={75,81,147}; p.ground={19,42,78}; p.horizon=0.82f; break;
    default: p.skyTop={19,12,62}; p.skyHorizon={151,73,158}; p.ground={20,26,78}; p.horizon=0.81f; break;
    }
    const int variantIndex = extraIndex < 60 ? extraIndex % 8 : (extraIndex - 60) % 8;
    const float variant = static_cast<float>(variantIndex);
    const float shift = (variant - 3.5f) * 3.5f;
    p.skyTop[0] = std::clamp(p.skyTop[0] + shift, 0.0f, 255.0f);
    p.skyTop[1] = std::clamp(p.skyTop[1] + (variantIndex % 3 - 1) * 7.0f, 0.0f, 255.0f);
    p.skyTop[2] = std::clamp(p.skyTop[2] - shift * 0.35f, 0.0f, 255.0f);
    p.skyHorizon[0] = std::clamp(p.skyHorizon[0] + shift * 1.5f, 0.0f, 255.0f);
    p.skyHorizon[1] = std::clamp(p.skyHorizon[1] + (variantIndex % 4 - 1) * 5.0f, 0.0f, 255.0f);
    p.skyHorizon[2] = std::clamp(p.skyHorizon[2] - shift * 0.5f, 0.0f, 255.0f);
    p.ground[0] = std::clamp(p.ground[0] + shift * 0.8f, 0.0f, 255.0f);
    p.ground[1] = std::clamp(p.ground[1] + (variantIndex % 3 - 1) * 5.0f, 0.0f, 255.0f);
    p.ground[2] = std::clamp(p.ground[2] - shift * 0.25f, 0.0f, 255.0f);
    p.horizon = std::clamp(p.horizon + (variantIndex % 5 - 2) * 0.008f, 0.58f, 0.92f);
    return p;
}

int environmentIndex(const std::string& name) {
    for (size_t i = 0; i < kEnvironmentNames.size(); ++i) {
        if (name == kEnvironmentNames[i]) return static_cast<int>(i);
    }
    return 0;
}
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


GLuint RoomRenderer::createProceduralEnvironment(int sceneIndex) {
    // Procedural window scenery is generated on demand. The first three scenes use bundled assets.
    constexpr int width = 512;
    constexpr int height = 256;
    std::vector<uint8_t> pixels(static_cast<size_t>(width * height * 3), 0);
    const auto hash32 = [](uint32_t value) {
        value ^= value >> 16;
        value *= 0x7feb352dU;
        value ^= value >> 15;
        value *= 0x846ca68bU;
        value ^= value >> 16;
        return value;
    };
    const auto byte = [](float value) {
        return static_cast<uint8_t>(std::clamp(value, 0.0f, 255.0f));
    };

    std::array<float, 3> skyTop{8.0f, 14.0f, 35.0f};
    std::array<float, 3> skyHorizon{40.0f, 75.0f, 112.0f};
    std::array<float, 3> ground{18.0f, 36.0f, 44.0f};
    float horizon = 0.70f;
    switch (sceneIndex) {
    case 3:  skyTop={4,7,22}; skyHorizon={16,31,57}; ground={3,7,17}; horizon=0.83f; break;             // aurora
    case 4:  skyTop={12,5,35}; skyHorizon={52,18,78}; ground={8,8,24}; horizon=0.84f; break;            // cyberpunk
    case 5:  skyTop={96,54,126}; skyHorizon={255,157,91}; ground={221,147,91}; horizon=0.62f; break;   // sunset beach
    case 6:  skyTop={16,50,95}; skyHorizon={151,207,232}; ground={183,218,237}; horizon=0.66f; break;  // snowy mountains
    case 7:  skyTop={7,24,30}; skyHorizon={40,95,78}; ground={7,27,24}; break;                           // enchanted forest
    case 8:  skyTop={72,40,32}; skyHorizon={255,183,95}; ground={209,132,60}; break;                      // desert
    case 9:  skyTop={12,50,83}; skyHorizon={89,178,198}; ground={15,56,77}; horizon=0.63f; break;        // ocean cliffs
    case 10: skyTop={3,4,18}; skyHorizon={35,20,80}; ground={4,5,19}; horizon=0.90f; break;              // nebula
    case 11: skyTop={5,15,38}; skyHorizon={69,91,127}; ground={91,98,112}; break;                         // lunar base
    case 12: skyTop={35,8,13}; skyHorizon={139,34,25}; ground={31,17,21}; break;                          // volcano
    case 13: skyTop={1,22,46}; skyHorizon={4,112,133}; ground={2,45,68}; horizon=0.94f; break;           // underwater
    case 14: skyTop={58,31,46}; skyHorizon={204,119,61}; ground={83,40,27}; break;                        // autumn
    case 15: skyTop={3,65,96}; skyHorizon={75,203,185}; ground={10,72,60}; horizon=0.62f; break;          // tropical
    case 16: skyTop={255,177,220}; skyHorizon={164,153,246}; ground={146,76,187}; break;                  // candy
    case 17: skyTop={36,20,23}; skyHorizon={164,108,62}; ground={42,26,25}; horizon=0.82f; break;         // steampunk
    case 18: skyTop={6,18,47}; skyHorizon={39,67,106}; ground={7,12,28}; horizon=0.83f; break;            // rainy tokyo
    case 19: skyTop={16,12,48}; skyHorizon={100,48,117}; ground={20,14,45}; horizon=0.84f; break;         // sakura night
    case 20: skyTop={28,68,107}; skyHorizon={143,211,229}; ground={24,83,103}; horizon=0.63f; break;      // alpine lake
    case 21: skyTop={14,54,47}; skyHorizon={103,174,102}; ground={26,74,43}; break;                      // emerald valley
    case 22: skyTop={39,31,29}; skyHorizon={168,111,70}; ground={57,43,36}; break;                        // wasteland
    case 23: skyTop={3,8,26}; skyHorizon={37,48,84}; ground={4,6,20}; horizon=0.87f; break;              // midnight clouds
    case 24: skyTop={12,6,47}; skyHorizon={80,43,128}; ground={24,13,49}; horizon=0.86f; break;          // crystal cavern
    case 25: skyTop={55,45,93}; skyHorizon={201,157,207}; ground={84,65,122}; break;                     // lavender fields
    case 26: skyTop={5,27,34}; skyHorizon={18,82,67}; ground={3,29,25}; break;                            // firefly swamp
    case 27: skyTop={8,35,60}; skyHorizon={139,188,207}; ground={103,130,150}; break;                    // polar station
    default: break;
    }
    if (sceneIndex >= 28 && sceneIndex < static_cast<int>(kEnvironmentNames.size())) {
        const auto palette = extraEnvironmentPalette(sceneIndex - 28);
        skyTop = palette.skyTop;
        skyHorizon = palette.skyHorizon;
        ground = palette.ground;
        horizon = palette.horizon;
    }

    const int addedPack = sceneIndex >= 88 ? (sceneIndex - 88) / 8 : -1;
    const bool city = sceneIndex == 4 || sceneIndex == 17 || sceneIndex == 18 ||
                      (sceneIndex >= 28 && sceneIndex <= 35) || addedPack == 11;
    const bool water = sceneIndex == 5 || sceneIndex == 9 || sceneIndex == 13 ||
                       sceneIndex == 15 || sceneIndex == 20 ||
                       (sceneIndex >= 44 && sceneIndex <= 51) || sceneIndex == 68 ||
                       sceneIndex == 91 || sceneIndex == 104 || sceneIndex == 105 || sceneIndex == 107 || sceneIndex == 111 ||
                       (sceneIndex >= 152 && sceneIndex <= 159) || sceneIndex == 133 ||
                       sceneIndex == 196 || sceneIndex == 198 || sceneIndex == 207;
    const bool forest = sceneIndex == 7 || sceneIndex == 14 || sceneIndex == 15 ||
                        sceneIndex == 19 || sceneIndex == 21 || sceneIndex == 26 ||
                        (sceneIndex >= 36 && sceneIndex <= 38) ||
                        (sceneIndex >= 40 && sceneIndex <= 43) ||
                        sceneIndex == 67 || sceneIndex == 76 || sceneIndex == 78 ||
                        sceneIndex == 81 || sceneIndex == 87 ||
                        sceneIndex == 106 || sceneIndex == 108 || sceneIndex == 109 || sceneIndex == 111 ||
                        sceneIndex == 112 || sceneIndex == 113 || sceneIndex == 115 || sceneIndex == 116 || sceneIndex == 118 ||
                        (sceneIndex >= 144 && sceneIndex <= 151) ||
                        (sceneIndex >= 192 && sceneIndex <= 199) ||
                        sceneIndex == 202 || sceneIndex == 204 || sceneIndex == 206;
    const bool mountain = sceneIndex == 6 || sceneIndex == 9 || sceneIndex == 11 ||
                          sceneIndex == 20 || sceneIndex == 21 || sceneIndex == 22 || sceneIndex == 27 ||
                          (sceneIndex >= 52 && sceneIndex <= 60) ||
                          (sceneIndex >= 69 && sceneIndex <= 70) ||
                          (sceneIndex >= 73 && sceneIndex <= 75) ||
                          sceneIndex == 120 || sceneIndex == 121 || sceneIndex == 122 ||
                          sceneIndex == 139 || sceneIndex == 143 || sceneIndex == 151 ||
                          sceneIndex == 197 || sceneIndex == 203 || sceneIndex == 205;
    const bool extraNight = sceneIndex == 28 || sceneIndex == 30 || sceneIndex == 31 ||
                            sceneIndex == 34 || sceneIndex == 35 || sceneIndex == 46 ||
                            sceneIndex == 51 || sceneIndex == 58 || sceneIndex == 59 ||
                            (sceneIndex >= 61 && sceneIndex <= 67) || sceneIndex == 71 ||
                            sceneIndex == 72 || sceneIndex == 75 || sceneIndex == 79 ||
                            sceneIndex == 80 || sceneIndex == 81 || sceneIndex == 82 ||
                            (sceneIndex >= 84 && sceneIndex <= 87) ||
                            sceneIndex == 90 || sceneIndex == 91 || sceneIndex == 92 || sceneIndex == 94 ||
                            sceneIndex == 97 || sceneIndex == 98 || sceneIndex == 101 || sceneIndex == 102 ||
                            sceneIndex == 128 || sceneIndex == 132 || sceneIndex == 133 ||
                            sceneIndex == 145 || sceneIndex == 150 ||
                            addedPack == 3 || addedPack == 9 || addedPack == 12 || addedPack == 14 ||
                            sceneIndex == 176 || sceneIndex == 180 || sceneIndex == 181 ||
                            sceneIndex == 198 || sceneIndex == 199;
    const bool night = sceneIndex == 3 || sceneIndex == 4 || sceneIndex == 10 ||
                       sceneIndex == 11 || sceneIndex == 18 || sceneIndex == 19 ||
                       sceneIndex == 23 || sceneIndex == 24 || sceneIndex == 26 || sceneIndex == 27 || extraNight;

    float sunX = 0.72f, sunY = 0.37f, sunRadius = 0.075f;
    std::array<float, 3> sunColor{255, 227, 177};
    switch (sceneIndex) {
    case 5: sunX=0.73f; sunY=0.42f; sunRadius=0.105f; sunColor={255,210,130}; break;
    case 6: sunX=0.70f; sunY=0.27f; sunRadius=0.050f; sunColor={226,242,255}; break;
    case 8: sunX=0.24f; sunY=0.36f; sunRadius=0.065f; sunColor={255,214,131}; break;
    case 10: sunX=0.68f; sunY=0.39f; sunRadius=0.10f; sunColor={160,118,255}; break;
    case 11: sunX=0.78f; sunY=0.22f; sunRadius=0.055f; sunColor={173,205,255}; break;
    case 19: sunX=0.72f; sunY=0.27f; sunRadius=0.055f; sunColor={255,218,239}; break;
    case 20: sunX=0.76f; sunY=0.28f; sunRadius=0.055f; sunColor={255,244,209}; break;
    case 25: sunX=0.29f; sunY=0.35f; sunRadius=0.080f; sunColor={255,220,218}; break;
    case 27: sunX=0.74f; sunY=0.26f; sunRadius=0.050f; sunColor={242,249,255}; break;
    default: break;
    }

    for (int y = 0; y < height; ++y) {
        const float v = static_cast<float>(y) / static_cast<float>(height - 1);
        for (int x = 0; x < width; ++x) {
            const float u = static_cast<float>(x) / static_cast<float>(width - 1);
            const uint32_t noise = hash32(static_cast<uint32_t>(x) * 374761393U
                                        + static_cast<uint32_t>(y) * 668265263U
                                        + static_cast<uint32_t>(sceneIndex) * 2246822519U);
            const float skyT = std::clamp(v / horizon, 0.0f, 1.0f);
            float r = skyTop[0] * (1.0f - skyT) + skyHorizon[0] * skyT;
            float g = skyTop[1] * (1.0f - skyT) + skyHorizon[1] * skyT;
            float b = skyTop[2] * (1.0f - skyT) + skyHorizon[2] * skyT;

            // Sky motifs: aurora curtains, nebula haze, layered midnight clouds and stars.
            if (sceneIndex == 3 && v < horizon) {
                const float center = 0.39f + 0.065f * std::sin(u * 7.0f)
                                   + 0.021f * std::sin(u * 23.0f);
                const float shimmer = 0.28f + 0.72f * (0.5f + 0.5f * std::sin(u * 19.0f + v * 31.0f));
                const float greenBand = std::exp(-std::pow((v - center) / 0.075f, 2.0f)) * shimmer;
                const float cyanBand = std::exp(-std::pow((v - center - 0.045f) / 0.10f, 2.0f));
                const float violetBand = std::exp(-std::pow((v - center - 0.13f) / 0.060f, 2.0f));
                r += 24.0f * greenBand + 11.0f * cyanBand + 78.0f * violetBand;
                g += 148.0f * greenBand + 47.0f * cyanBand + 17.0f * violetBand;
                b += 88.0f * greenBand + 115.0f * cyanBand + 150.0f * violetBand;
            }
            if ((sceneIndex == 10 || sceneIndex == 23) && v < 0.75f) {
                const float cloud = std::max(0.0f, std::sin(u * 12.0f + v * 10.0f)
                                                   * std::sin(u * 5.0f - v * 17.0f));
                const float glow = cloud * (sceneIndex == 10 ? 0.55f : 0.25f);
                r += glow * (sceneIndex == 10 ? 96.0f : 23.0f);
                g += glow * (sceneIndex == 10 ? 25.0f : 35.0f);
                b += glow * (sceneIndex == 10 ? 143.0f : 70.0f);
            }
            if (night && v < 0.67f && noise % 2048U < 3U) {
                const float star = 95.0f + static_cast<float>((noise >> 9) & 0x7fU);
                r += star * 0.78f; g += star * 0.86f; b += star;
            }
            if ((sceneIndex == 58 || sceneIndex == 59) && v < horizon) {
                const float center = 0.39f + 0.065f * std::sin(u * 7.0f + 0.6f)
                                   + 0.021f * std::sin(u * 23.0f);
                const float ribbon = std::exp(-std::pow((v - center) / 0.075f, 2.0f));
                const float wave = 0.45f + 0.55f * (0.5f + 0.5f * std::sin(u * 19.0f + v * 31.0f));
                r += (sceneIndex == 58 ? 13.0f : 23.0f) * ribbon * wave;
                g += (sceneIndex == 58 ? 126.0f : 73.0f) * ribbon * wave;
                b += (sceneIndex == 58 ? 98.0f : 151.0f) * ribbon * wave;
            }
            if ((sceneIndex == 5 || sceneIndex == 6 || sceneIndex == 8 || sceneIndex == 10 ||
                 sceneIndex == 11 || sceneIndex == 19 || sceneIndex == 20 ||
                 sceneIndex == 25 || sceneIndex == 27 || (sceneIndex >= 28 && !night && addedPack != 1 && addedPack != 12)) && v < horizon) {
                const float dx = (u - sunX) * 1.6f;
                const float dy = v - sunY;
                if (dx * dx + dy * dy < sunRadius * sunRadius) {
                    r = sunColor[0]; g = sunColor[1]; b = sunColor[2];
                }
            }

            const float waveHorizon = horizon + 0.018f * std::sin(u * 13.0f)
                                               + 0.008f * std::sin(u * 31.0f);
            if (v > waveHorizon) {
                r = ground[0]; g = ground[1]; b = ground[2];
            }

            // Layered distant mountain silhouettes. Snow scenes use bright ridgelines.
            if (mountain) {
                const float ridgeBack = 0.48f + 0.055f * std::sin(u * 8.0f + 0.7f)
                                      + 0.035f * std::sin(u * 19.0f);
                const float ridgeFront = 0.60f + 0.065f * std::sin(u * 5.0f + 1.1f)
                                       + 0.025f * std::sin(u * 17.0f);
                if (v > ridgeBack && v < 0.82f) {
                    r = ground[0] * 0.74f; g = ground[1] * 0.78f; b = ground[2] * 0.84f;
                }
                if (v > ridgeFront && v < 0.86f) {
                    r = ground[0] * 0.95f; g = ground[1] * 0.90f; b = ground[2] * 0.86f;
                }
                if ((sceneIndex == 6 || sceneIndex == 20 || sceneIndex == 27 ||
                     (sceneIndex >= 52 && sceneIndex <= 59)) &&
                    v > ridgeBack && v < ridgeBack + 0.035f) {
                    r = 231.0f; g = 244.0f; b = 250.0f;
                }
                if (sceneIndex == 9 && ((u < 0.20f && v > 0.43f) || (u > 0.82f && v > 0.48f))) {
                    r = 35.0f; g = 53.0f; b = 62.0f;
                }
                if (sceneIndex == 11 && v > 0.70f) {
                    const float crater = std::sin(u * 45.0f) * std::sin(v * 57.0f);
                    if (std::fabs(crater) < 0.09f) { r *= 0.70f; g *= 0.70f; b *= 0.70f; }
                }
                if (sceneIndex == 27 && v > 0.73f && u > 0.58f && u < 0.78f) {
                    r = 27.0f; g = 50.0f; b = 65.0f;
                    if (static_cast<int>(v * 220.0f) % 13 < 3) { r = 230.0f; g = 193.0f; b = 112.0f; }
                }
            }

            // Lakes, sea, shoreline, and undersea haze.
            if (water && v > horizon) {
                const float wave = std::sin(v * 176.0f + u * 38.0f)
                                 + 0.45f * std::sin(v * 93.0f - u * 19.0f);
                const float shimmer = wave > 1.05f ? 22.0f : wave > 0.65f ? 9.0f : 0.0f;
                r = ground[0] + shimmer * ((sceneIndex == 5 || sceneIndex == 44 || sceneIndex == 45 || sceneIndex == 50) ? 1.0f : 0.3f);
                g = ground[1] + shimmer * 0.9f;
                b = ground[2] + shimmer;
                if (sceneIndex == 5 && v > 0.87f) {
                    r = 208.0f; g = 137.0f; b = 86.0f;
                }
                if (sceneIndex == 15) {
                    const float islandLine = 0.62f + 0.055f * std::sin(u * 8.0f);
                    if (std::fabs(v - islandLine) < 0.045f && u > 0.22f && u < 0.83f) {
                        r = 32.0f; g = 94.0f; b = 61.0f;
                    }
                }
                if (sceneIndex == 13 || sceneIndex == 44 || sceneIndex == 51) {
                    const float bubbleU = static_cast<float>((noise >> 8) % 1000U) / 1000.0f;
                    if ((x % 67 < 3) && std::fabs(u - bubbleU) < 0.012f) { r = 93.0f; g = 219.0f; b = 237.0f; }
                    if (std::fabs(std::sin(u * 21.0f + v * 6.0f)) < 0.035f && v > 0.40f) {
                        r += 20.0f; g += 27.0f; b += 25.0f;
                    }
                }
            }

            // Forest canopies make the silhouettes distinct from simple color gradients.
            if (forest) {
                const float repeating = std::fabs(std::fmod(u * (sceneIndex == 15 ? 18.0f : 31.0f), 1.0f) - 0.5f) * 2.0f;
                const float treeTop = 0.55f + repeating * (sceneIndex == 15 ? 0.20f : 0.14f)
                                    + 0.025f * std::sin(u * 37.0f);
                if (v > treeTop && v < 0.92f) {
                    if (sceneIndex == 14 || sceneIndex == 78) { r = 113.0f + 38.0f * repeating; g = 45.0f + 27.0f * repeating; b = 25.0f; }
                    else if (sceneIndex == 15 || sceneIndex == 40) { r = 8.0f; g = 56.0f + 22.0f * repeating; b = 38.0f; }
                    else if (sceneIndex == 26 || sceneIndex == 43) { r = 3.0f; g = 35.0f + 15.0f * repeating; b = 31.0f; }
                    else if (sceneIndex == 21 || sceneIndex == 36) { r = 15.0f + 30.0f * repeating; g = 49.0f + 35.0f * repeating; b = 35.0f; }
                    else { r = 6.0f; g = 40.0f + 25.0f * repeating; b = 36.0f; }
                }
                if ((sceneIndex == 19 || sceneIndex == 81) && v > 0.52f && v < 0.74f &&
                    (std::fabs(std::sin(u * 21.0f + v * 10.0f)) < 0.13f) &&
                    noise % 5U == 0U) {
                    r = 255.0f; g = 177.0f; b = 222.0f;
                }
                if (sceneIndex == 26 && v < 0.82f && noise % 1400U < 2U) {
                    r = 255.0f; g = 224.0f; b = 112.0f;
                }
            }

            // Dune curves, ash fields, lava seams and alien wasteland contours.
            if (sceneIndex == 8 || sceneIndex == 22 || sceneIndex == 69 || sceneIndex == 70) {
                const float dune = 0.57f + 0.10f * std::sin(u * 8.0f + 0.4f)
                                 + 0.045f * std::sin(u * 21.0f);
                if (v > dune) {
                    if (sceneIndex == 8 || sceneIndex == 69 || sceneIndex == 70) { r = (sceneIndex == 69 ? 176.0f : 218.0f) + 17.0f * std::sin(u * 4.0f); g = sceneIndex == 70 ? 112.0f : 139.0f; b = sceneIndex == 69 ? 59.0f : 67.0f; }
                    else { r = 63.0f; g = 48.0f; b = 42.0f; }
                }
            }
            if (sceneIndex == 12 || sceneIndex == 71 || sceneIndex == 72) {
                const float volcanoPeak = 0.43f + std::fabs(u - 0.50f) * 0.78f;
                if (v > volcanoPeak) {
                    r = sceneIndex == 71 ? 53.0f : 31.0f; g = 20.0f; b = 25.0f;
                    const float crack = std::fabs(std::fmod(u * (sceneIndex == 72 ? 31.0f : 17.0f) + std::sin(v * 15.0f) * 0.18f, 1.0f) - 0.5f);
                    if (v > 0.72f && crack < 0.022f) { r = 255.0f; g = sceneIndex == 71 ? 34.0f : 71.0f; b = 18.0f; }
                }
                if ((sceneIndex == 12 || sceneIndex == 71) && v > 0.43f && v < 0.55f && std::fabs(u - 0.50f) < 0.045f) {
                    r = 255.0f; g = 103.0f; b = 30.0f;
                }
            }

            // Candy hills, striped lavender rows and a glowing crystal cavern.
            if (sceneIndex == 16) {
                const float hill = 0.58f + 0.12f * std::sin(u * 8.0f) + 0.035f * std::sin(u * 23.0f);
                if (v > hill) {
                    const int stripe = (static_cast<int>(u * 24.0f) + static_cast<int>(v * 6.0f)) % 4;
                    const float colors[4][3] = {{245,113,183},{255,198,105},{117,218,235},{145,105,222}};
                    r = colors[stripe][0]; g = colors[stripe][1]; b = colors[stripe][2];
                }
            }
            if (sceneIndex == 24 || sceneIndex == 64 || sceneIndex == 87) {
                const float topSpike = 0.12f + 0.16f * std::fabs(std::sin(u * 23.0f));
                const float bottomSpike = 0.83f - 0.16f * std::fabs(std::sin(u * 17.0f + 0.7f));
                if (v < topSpike || v > bottomSpike) {
                    const bool cyan = (static_cast<int>(u * 42.0f) % 3) == 0;
                    r = cyan ? 29.0f : 110.0f; g = cyan ? 222.0f : 75.0f; b = cyan ? 247.0f : 235.0f;
                } else {
                    const float crystalGlow = std::max(0.0f, std::sin(u * 24.0f + v * 5.0f));
                    r += 24.0f * crystalGlow; g += 8.0f * crystalGlow; b += 40.0f * crystalGlow;
                }
            }
            if ((sceneIndex == 25 || sceneIndex == 82) && v > 0.63f) {
                const int row = static_cast<int>((v - 0.63f) * 220.0f);
                const int plant = static_cast<int>(u * 160.0f) % 5;
                r = plant % 2 == 0 ? 115.0f : 157.0f;
                g = 75.0f + static_cast<float>(row % 12) * 2.0f;
                b = 155.0f + static_cast<float>(plant) * 12.0f;
                if (row % 17 < 3) { r = 211.0f; g = 170.0f; b = 231.0f; }
            }

            if (city) {
                const int building = x / 16;
                const uint32_t seed = hash32(static_cast<uint32_t>(building) * 7919U
                                           + static_cast<uint32_t>(sceneIndex) * 113U + 37U);
                const float roof = 0.43f + static_cast<float>((seed >> 8) % 320U) / 1000.0f;
                if (v > roof) {
                    r = 8.0f + static_cast<float>(seed & 7U);
                    g = 8.0f + static_cast<float>((seed >> 3) & 7U);
                    b = 22.0f + static_cast<float>((seed >> 6) & 23U);
                    const int localX = x % 16;
                    const int row = static_cast<int>((v - roof) * 360.0f) % 18;
                    const uint32_t windowSeed = hash32(static_cast<uint32_t>(building * 113 + row * 31 + sceneIndex));
                    if (localX >= 3 && localX <= 11 && row >= 3 && row <= 12 && windowSeed % 7U < 3U) {
                        if (sceneIndex == 17) { r = 231.0f; g = 158.0f; b = 63.0f; }
                        else if (sceneIndex == 18) { r = 32.0f; g = 211.0f; b = 255.0f; }
                        else if (windowSeed & 1U) { r = 240.0f; g = 43.0f; b = 165.0f; }
                        else { r = 24.0f; g = 216.0f; b = 255.0f; }
                    } else if (localX == 0) { r = 29.0f; g = 31.0f; b = 68.0f; }
                }
                if (sceneIndex == 18 && v < 0.82f && (x % 37) < 2 &&
                    (noise % 5U == 0U)) { r = 51.0f; g = 91.0f; b = 137.0f; }
            }

            // Dreamcore: soft cloud haze and oversized pastel moons.
            if ((addedPack == 0 || addedPack == 10 || addedPack == 12) && v < horizon) {
                const float cloud = std::max(0.0f,
                    std::sin(u * 13.0f + std::sin(v * 17.0f)) *
                    std::sin(v * 26.0f + u * 5.0f));
                const float strength = cloud * (addedPack == 10 ? 27.0f : 16.0f);
                r += strength * (addedPack == 10 ? 1.45f : 0.92f);
                g += strength * (addedPack == 10 ? 0.68f : 0.78f);
                b += strength * 1.35f;
            }
            if (sceneIndex == 88 || sceneIndex == 89 || sceneIndex == 95 || sceneIndex == 184 || sceneIndex == 187) {
                const float moonX = sceneIndex == 184 ? 0.50f : sceneIndex == 89 ? 0.36f : 0.72f;
                const float moonY = sceneIndex == 184 ? 0.37f : 0.29f;
                const float dx = (u - moonX) * 1.45f, dy = v - moonY;
                const float d2 = dx * dx + dy * dy;
                const float radius = sceneIndex == 184 ? 0.20f : 0.070f;
                if (d2 < radius * radius) {
                    r = sceneIndex == 184 ? 238.0f : 240.0f;
                    g = sceneIndex == 184 ? 213.0f : 223.0f;
                    b = sceneIndex == 184 ? 255.0f : 246.0f;
                    if (d2 > radius * radius * 0.68f) { r -= 28.0f; g -= 20.0f; b -= 8.0f; }
                } else if (d2 < radius * radius * 2.3f) { r += 24.0f; g += 17.0f; b += 34.0f; }
            }
            // Empty halls and impossible dream spaces get stylized perspective lines.
            if (sceneIndex == 90 || sceneIndex == 92 || sceneIndex == 94 ||
                sceneIndex == 185 || sceneIndex == 187 || sceneIndex == 188 ||
                sceneIndex == 189 || sceneIndex == 190 || sceneIndex == 191) {
                const float depth = std::clamp((v - 0.43f) * 2.2f, 0.0f, 1.0f);
                const float horizontalPhase = std::fabs(std::fmod((v - 0.43f) * 17.0f, 1.0f) - 0.5f);
                const float rayPhase = std::fabs(std::fmod(std::fabs(u - 0.5f) * (1.0f + depth * 5.0f), 0.18f) - 0.09f);
                if (v > 0.48f && (horizontalPhase < 0.035f || rayPhase < 0.009f)) {
                    r += 40.0f; g += 36.0f; b += 57.0f;
                }
            }
            // Liminal poolrooms use turquoise water and a tiled grid.
            if (sceneIndex == 91 && v > horizon) {
                const float depth = (v - horizon) / (1.0f - horizon);
                const bool tileLine = std::fabs(std::fmod(depth * 19.0f, 1.0f) - 0.5f) < 0.055f ||
                    std::fabs(std::fmod(u * 30.0f + depth * 8.0f, 1.0f) - 0.5f) < 0.035f;
                if (tileLine) { r = 87.0f; g = 210.0f; b = 207.0f; }
                else { r = 13.0f + depth * 6.0f; g = 128.0f + depth * 18.0f; b = 145.0f + depth * 15.0f; }
            }
            // Gentle rain, snow, and warm lights make the comfort collection feel lived in.
            if (sceneIndex == 97 || sceneIndex == 136 || sceneIndex == 137 ||
                sceneIndex == 138 || sceneIndex == 145 || sceneIndex == 181) {
                if (v < 0.88f && ((x + static_cast<int>((noise >> 7) % 11U)) % 37) < 2 && noise % 4U != 0U) {
                    r += 14.0f; g += 23.0f; b += 32.0f;
                }
            }
            if ((sceneIndex == 96 || sceneIndex == 128 || sceneIndex == 135 || sceneIndex == 142) &&
                v < 0.92f && noise % 283U < 4U) {
                r = std::min(255.0f, r + 62.0f);
                g = std::min(255.0f, g + 64.0f);
                b = std::min(255.0f, b + 70.0f);
            }
            if (sceneIndex >= 96 && sceneIndex <= 103 && noise % 191U < 3U && v > 0.34f && v < 0.89f) {
                r = std::min(255.0f, r + 55.0f);
                g = std::min(255.0f, g + 32.0f);
                b = std::min(255.0f, b + 12.0f);
            }
            // Fairy lights, fireflies, lanterns, and small magical glows.
            if ((addedPack == 3 || addedPack == 14) && noise % 470U < 3U && v > 0.16f && v < 0.88f) {
                r += 95.0f; g += 80.0f; b += 112.0f;
            }
            if ((sceneIndex == 132 || sceneIndex == 133 || sceneIndex == 134 || sceneIndex == 150) &&
                noise % 170U < 3U && v > 0.24f && v < 0.86f) {
                r += 100.0f; g += 64.0f; b += 24.0f;
            }
            if ((sceneIndex == 149 || sceneIndex == 206) && v > 0.25f && v < 0.82f && noise % 97U < 3U) {
                r = 255.0f; g = 184.0f; b = 221.0f;
            }
            // Bioluminescent particles and jellyfish-like specks.
            if (addedPack == 8 && v > 0.20f && noise % 230U < 4U) {
                r += 24.0f; g += 82.0f; b += 106.0f;
            }
            // Large planetary disks and ring systems for the cosmic pack.
            if (addedPack == 9 && (sceneIndex == 160 || sceneIndex == 162 || sceneIndex == 164 || sceneIndex == 165)) {
                const float planetX = sceneIndex == 160 ? 0.53f : 0.72f;
                const float planetY = sceneIndex == 165 ? 0.35f : 0.40f;
                const float dx = (u - planetX) * 1.5f, dy = v - planetY;
                const float d2 = dx * dx + dy * dy;
                const float radius = sceneIndex == 160 ? 0.16f : 0.105f;
                if (sceneIndex == 160 && std::fabs(dy + dx * 0.18f) < 0.018f && d2 < radius * radius * 2.1f) {
                    r = 227.0f; g = 186.0f; b = 151.0f;
                }
                if (d2 < radius * radius) {
                    r = 88.0f + 60.0f * (1.0f - d2 / (radius * radius));
                    g = sceneIndex == 165 ? 146.0f : 76.0f;
                    b = sceneIndex == 164 ? 181.0f : 214.0f;
                }
            }
            // Pastel hills, rainbow reflections, and cotton-candy bands.
            if (addedPack == 10 && v > horizon) {
                const int stripe = (static_cast<int>(u * 19.0f) + static_cast<int>(v * 9.0f)) % 6;
                static constexpr float hues[6][3] = {{249,153,195},{255,207,154},{255,241,167},{158,231,208},{156,195,255},{202,165,245}};
                if (sceneIndex != 171 || stripe < 4) {
                    r = hues[stripe][0] * 0.64f + ground[0] * 0.36f;
                    g = hues[stripe][1] * 0.64f + ground[1] * 0.36f;
                    b = hues[stripe][2] * 0.64f + ground[2] * 0.36f;
                }
            }
            if (addedPack == 13 && (sceneIndex == 193 || sceneIndex == 196 || sceneIndex == 198) && v > horizon) {
                const float reflection = 0.5f + 0.5f * std::sin(v * 88.0f + u * 9.0f);
                r += reflection * 13.0f; g += reflection * 20.0f; b += reflection * 19.0f;
            }
            if (addedPack == 14 && v < horizon) {
                const float band = 0.36f + 0.055f * std::sin(u * 8.0f) + 0.018f * std::sin(u * 21.0f);
                const float glow = std::exp(-std::pow((v - band) / 0.08f, 2.0f));
                r += 24.0f * glow; g += 100.0f * glow; b += 116.0f * glow;
            }

            if ((sceneIndex == 80 || sceneIndex == 81) && v < 0.72f && noise % 130U < 3U) {
                if (sceneIndex == 80) { r += 142.0f; g += 84.0f; b += 28.0f; }
                else { r += 132.0f; g += 70.0f; b += 121.0f; }
            }
            if ((sceneIndex == 85 || sceneIndex == 86) && v > horizon) {
                const float depth = (v - horizon) / (1.0f - horizon);
                const bool horizontalGrid = std::fabs(std::fmod(depth * 19.0f, 1.0f) - 0.5f) < 0.035f;
                const bool verticalGrid = std::fabs(std::fmod(u * 30.0f + depth * 8.0f, 1.0f) - 0.5f) < 0.025f;
                if (horizontalGrid || verticalGrid) { r += 70.0f; g += 15.0f; b += 120.0f; }
            }
            if (sceneIndex == 62 && v < horizon) {
                for (int starIndex = 0; starIndex < 2; ++starIndex) {
                    const float sx = starIndex == 0 ? 0.31f : 0.70f;
                    const float sy = starIndex == 0 ? 0.34f : 0.39f;
                    const float radius = starIndex == 0 ? 0.052f : 0.035f;
                    const float dx = (u - sx) * 1.6f;
                    if (dx * dx + (v - sy) * (v - sy) < radius * radius) {
                        r = starIndex == 0 ? 255.0f : 129.0f;
                        g = starIndex == 0 ? 226.0f : 191.0f;
                        b = starIndex == 0 ? 171.0f : 255.0f;
                    }
                }
            }

            // Aurora/city/lunar nights retain stars above their horizon.
            if (sceneIndex == 23 && v > 0.25f && v < 0.55f) {
                const float cloudEdge = 0.35f + 0.045f * std::sin(u * 18.0f);
                if (std::fabs(v - cloudEdge) < 0.025f) { r += 18.0f; g += 22.0f; b += 38.0f; }
            }
            const size_t at = static_cast<size_t>((y * width + x) * 3);
            pixels[at] = byte(r);
            pixels[at + 1] = byte(g);
            pixels[at + 2] = byte(b);
        }
    }

    GLuint texture = 0;
    glGenTextures(1, &texture);
    if (!texture) return 0;
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    LOGI("generated scenery %s (%d x %d)", kEnvironmentNames[sceneIndex], width, height);
    return texture;
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

    // Smooth UV sphere mesh used for rounded cushions, chair details and PC cooling fans.
    // It is generated once and shared by every room object to keep draw overhead low.
    std::vector<float> sphere;
    constexpr int rings = 12, segments = 20;
    sphere.reserve(static_cast<size_t>(rings * segments * 6 * 5));
    auto vertex = [&](float lat, float lon) {
        const float cl = std::cos(lat);
        return std::array<float,5>{cl*std::cos(lon), std::sin(lat), cl*std::sin(lon),
                                   lon/(2.0f*kPi)+0.5f, lat/kPi+0.5f};
    };
    for (int r = 0; r < rings; ++r) {
        const float lat0 = -kPi*0.5f + kPi*float(r)/rings;
        const float lat1 = -kPi*0.5f + kPi*float(r+1)/rings;
        for (int s = 0; s < segments; ++s) {
            const float lon0 = 2.0f*kPi*float(s)/segments;
            const float lon1 = 2.0f*kPi*float(s+1)/segments;
            const auto a=vertex(lat0,lon0), b=vertex(lat0,lon1), d=vertex(lat1,lon0), e=vertex(lat1,lon1);
            for (const auto& v : {a,d,b,b,d,e}) sphere.insert(sphere.end(),v.begin(),v.end());
        }
    }
    sphereVertexCount_ = static_cast<GLsizei>(sphere.size()/5);
    glGenVertexArrays(1,&sphereVao_);
    glGenBuffers(1,&sphereVbo_);
    glBindVertexArray(sphereVao_);
    glBindBuffer(GL_ARRAY_BUFFER,sphereVbo_);
    glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(sphere.size()*sizeof(float)),sphere.data(),GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,5*sizeof(float),reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,5*sizeof(float),reinterpret_cast<void*>(3*sizeof(float)));
    glBindVertexArray(0);
}

void RoomRenderer::setSettings(const RoomSettings& settings) { settings_=settings; chooseEnvironment(); }
void RoomRenderer::chooseEnvironment() {
    const int previousIndex = activeEnvironmentIndex_;
    const int selectedIndex = environmentIndex(settings_.environment);
    // Keep the 3 bundled images and the currently selected procedural image resident.
    if (initialized_ && previousIndex != selectedIndex && previousIndex >= 3 &&
        previousIndex < static_cast<int>(environmentTextures_.size()) && environmentTextures_[previousIndex]) {
        glDeleteTextures(1, &environmentTextures_[previousIndex]);
        environmentTextures_[previousIndex] = 0;
    }
    activeEnvironmentIndex_ = selectedIndex;
    if (activeEnvironmentIndex_ >= 3 && initialized_ && !environmentTextures_[activeEnvironmentIndex_]) {
        environmentTextures_[activeEnvironmentIndex_] = createProceduralEnvironment(activeEnvironmentIndex_);
        if (!environmentTextures_[activeEnvironmentIndex_]) {
            LOGW("Could not generate scenery %s; the room window will use its fallback.", settings_.environment.c_str());
        }
    }
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
    const int nextIndex = (activeEnvironmentIndex_ + 1) % static_cast<int>(kEnvironmentNames.size());
    settings_.environment = kEnvironmentNames[nextIndex];
    chooseEnvironment();
    LOGI("Room scenery changed to %s", settings_.environment.c_str());
}

void RoomRenderer::toggleMenu() {
    menuVisible_ = !menuVisible_;
    previousMenuNavigation_ = false;
    if (menuVisible_) menuSelection_ = 0;
}

void RoomRenderer::handleMenuInput(float vertical, bool selectPressed) {
    if (!menuVisible_) return;
    const bool navigating = std::fabs(vertical) > 0.55f;
    if (navigating && !previousMenuNavigation_) {
        menuSelection_ = (menuSelection_ + (vertical > 0.0f ? 4 : 1)) % 5;
    }
    previousMenuNavigation_ = navigating;
    if (!selectPressed) return;
    switch (menuSelection_) {
    case 0: cycleEnvironment(); break;
    case 1: cycleScreenDestination(); break;
    case 2: settings_.showTv = !settings_.showTv; break;
    case 3: settings_.showCouch = !settings_.showCouch; break;
    case 4: menuVisible_ = false; break;
    default: break;
    }
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
    const float deskX=halfW-1.48f, deskZ=-halfD+1.35f;
    const Seat seats[] = {
        {deskX-.25f, deskZ+1.20f, 0.0f, "desk chair", settings_.showChair},
        {halfW-1.25f, .05f, -kPi*.5f, "couch", settings_.showCouch}
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
    // Real headset tracking remains untouched; this only positions the virtual world.
    moveX_=nearest->x-headPose.position.x;
    moveZ_=nearest->z-headPose.position.z;
    worldOffsetX_ = 0.0f;
    worldOffsetZ_ = 0.0f;
    yawPivotX_ = headPose.position.x + moveX_;
    yawPivotY_ = headPose.position.y;
    yawPivotZ_ = headPose.position.z + moveZ_;
    yawPivotInitialized_ = true;
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
void RoomRenderer::drawText(const float* pv, const char* text, float x, float y, float z,
                            float pixelSize, const std::array<float,4>& color) {
    if (!text) return;
    auto glyph = [](char c) {
        switch (c) {
        case 'A': return std::array<uint8_t,7>{14,17,17,31,17,17,17};
        case 'B': return std::array<uint8_t,7>{30,17,17,30,17,17,30};
        case 'C': return std::array<uint8_t,7>{14,17,16,16,16,17,14};
        case 'D': return std::array<uint8_t,7>{30,17,17,17,17,17,30};
        case 'E': return std::array<uint8_t,7>{31,16,16,30,16,16,31};
        case 'F': return std::array<uint8_t,7>{31,16,16,30,16,16,16};
        case 'G': return std::array<uint8_t,7>{14,17,16,23,17,17,15};
        case 'H': return std::array<uint8_t,7>{17,17,17,31,17,17,17};
        case 'I': return std::array<uint8_t,7>{14,4,4,4,4,4,14};
        case 'J': return std::array<uint8_t,7>{7,2,2,2,18,18,12};
        case 'K': return std::array<uint8_t,7>{17,18,20,24,20,18,17};
        case 'L': return std::array<uint8_t,7>{16,16,16,16,16,16,31};
        case 'M': return std::array<uint8_t,7>{17,27,21,21,17,17,17};
        case 'N': return std::array<uint8_t,7>{17,25,21,19,17,17,17};
        case 'O': return std::array<uint8_t,7>{14,17,17,17,17,17,14};
        case 'P': return std::array<uint8_t,7>{30,17,17,30,16,16,16};
        case 'Q': return std::array<uint8_t,7>{14,17,17,17,21,18,13};
        case 'R': return std::array<uint8_t,7>{30,17,17,30,20,18,17};
        case 'S': return std::array<uint8_t,7>{15,16,16,14,1,1,30};
        case 'T': return std::array<uint8_t,7>{31,4,4,4,4,4,4};
        case 'U': return std::array<uint8_t,7>{17,17,17,17,17,17,14};
        case 'V': return std::array<uint8_t,7>{17,17,17,17,17,10,4};
        case 'W': return std::array<uint8_t,7>{17,17,17,21,21,21,10};
        case 'X': return std::array<uint8_t,7>{17,17,10,4,10,17,17};
        case 'Y': return std::array<uint8_t,7>{17,17,10,4,4,4,4};
        case 'Z': return std::array<uint8_t,7>{31,1,2,4,8,16,31};
        case ':': return std::array<uint8_t,7>{0,4,4,0,4,4,0};
        case '-': return std::array<uint8_t,7>{0,0,0,31,0,0,0};
        default: return std::array<uint8_t,7>{0,0,0,0,0,0,0};
        }
    };
    std::vector<Vertex> vertices;
    vertices.reserve(std::strlen(text) * 30u * 6u);
    auto addVertex = [&](float px, float py) { vertices.push_back({px, py, z, 0.0f, 0.0f}); };
    for (int index = 0; text[index] != '\0'; ++index) {
        const char character = text[index] == '_' ? ' ' : static_cast<char>(std::toupper(static_cast<unsigned char>(text[index])));
        const auto rows = glyph(character);
        for (int row = 0; row < 7; ++row) for (int col = 0; col < 5; ++col) {
            if ((rows[row] & (1u << (4 - col))) == 0) continue;
            const float x0 = x + static_cast<float>(index) * pixelSize * 6.0f + col * pixelSize;
            const float y0 = y - row * pixelSize;
            const float x1 = x0 + pixelSize * 0.78f;
            const float y1 = y0 - pixelSize * 0.78f;
            addVertex(x0,y0); addVertex(x1,y0); addVertex(x1,y1);
            addVertex(x0,y0); addVertex(x1,y1); addVertex(x0,y1);
        }
    }
    if (vertices.empty()) return;
    if (!textVao_) {
        glGenVertexArrays(1, &textVao_);
        glGenBuffers(1, &textVbo_);
        glBindVertexArray(textVao_);
        glBindBuffer(GL_ARRAY_BUFFER, textVbo_);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              reinterpret_cast<void*>(3 * sizeof(float)));
        glBindVertexArray(0);
    }
    glUniformMatrix4fv(locMvp_, 1, GL_FALSE, pv);
    glUniform4fv(locColor_, 1, color.data());
    glUniform1i(locTextured_, 0);
    glBindVertexArray(textVao_);
    glBindBuffer(GL_ARRAY_BUFFER, textVbo_);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void RoomRenderer::drawSphere(const float* pv,float x,float y,float z,float sx,float sy,float sz,const std::array<float,4>& color) {
    Mat4 projectionView{}; std::copy(pv,pv+projectionView.size(),projectionView.begin());
    const Mat4 model=mul(translation(x,y,z),scaling(sx,sy,sz));
    const Mat4 mvp=mul(projectionView,model);
    glUniformMatrix4fv(locMvp_,1,GL_FALSE,mvp.data());
    glUniform4fv(locColor_,1,color.data());
    glUniform1i(locTextured_,0);
    glBindVertexArray(sphereVao_);
    glDrawArrays(GL_TRIANGLES,0,sphereVertexCount_);
    glBindVertexArray(0);
}
void RoomRenderer::drawPlane(const float* pv,float x,float y,float z,float sx,float sy,GLuint texture,const std::array<float,4>& color,float yaw) {
    Mat4 projectionView{}; std::copy(pv, pv + projectionView.size(), projectionView.begin());
    Mat4 model=mul(translation(x,y,z),mul(rotateY(yaw),scaling(sx,sy,1)));Mat4 mvp=mul(projectionView,model);
    glUniformMatrix4fv(locMvp_,1,GL_FALSE,mvp.data());glUniform4fv(locColor_,1,color.data());glUniform1i(locTextured_,texture?1:0);
    glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,texture?texture:videoFallbackTexture_);glUniform1i(locTexture_,0);
    glBindVertexArray(planeTexture_);glDrawArrays(GL_TRIANGLES,0,6);glBindVertexArray(0);glBindTexture(GL_TEXTURE_2D,0);
}

void RoomRenderer::drawCurvedPanel(const float* pv, float x, float y, float z,
                                    float width, float height, float curveDepth, GLuint texture,
                                    const std::array<float,4>& color, int segments) {
    segments = std::clamp(segments, 8, 48);
    std::vector<Vertex> vertices;
    vertices.reserve(static_cast<size_t>(segments) * 6u);
    auto point = [&](float u, float v) {
        const float localX = (u - 0.5f) * width;
        const float normalized = 2.0f * u - 1.0f;
        const float localZ = curveDepth * (1.0f - normalized * normalized);
        return Vertex{localX, (v - 0.5f) * height, localZ, u, 1.0f - v};
    };
    for (int i = 0; i < segments; ++i) {
        const float u0 = static_cast<float>(i) / static_cast<float>(segments);
        const float u1 = static_cast<float>(i + 1) / static_cast<float>(segments);
        const Vertex bl = point(u0, 0.0f), br = point(u1, 0.0f);
        const Vertex tr = point(u1, 1.0f), tl = point(u0, 1.0f);
        vertices.insert(vertices.end(), {bl, br, tr, bl, tr, tl});
    }
    if (!curvedVao_) {
        glGenVertexArrays(1, &curvedVao_);
        glGenBuffers(1, &curvedVbo_);
        glBindVertexArray(curvedVao_);
        glBindBuffer(GL_ARRAY_BUFFER, curvedVbo_);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              reinterpret_cast<void*>(3 * sizeof(float)));
        glBindVertexArray(0);
    }
    const Mat4 model = translation(x, y, z);
    Mat4 projectionView{};
    std::copy(pv, pv + projectionView.size(), projectionView.begin());
    const Mat4 mvp = mul(projectionView, model);
    glUniformMatrix4fv(locMvp_, 1, GL_FALSE, mvp.data());
    glUniform4fv(locColor_, 1, color.data());
    glUniform1i(locTextured_, texture ? 1 : 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture ? texture : videoFallbackTexture_);
    glUniform1i(locTexture_, 0);
    glBindVertexArray(curvedVao_);
    glBindBuffer(GL_ARRAY_BUFFER, curvedVbo_);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
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
    if (!yawPivotInitialized_) rebaseYawPivot(headPose);
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
    const float deskX=halfW-1.48f, deskZ=-halfD+1.35f;
    const float couchX=halfW-1.25f, couchZ=.05f;
    const bool hitsDesk=overlaps(moveX_,moveZ_,deskX,deskZ,1.45f,.70f);
    const bool hitsChair=settings_.showChair && overlaps(moveX_,moveZ_,deskX-.25f,deskZ+1.20f,.55f,.55f);
    const bool hitsCouch=settings_.showCouch && overlaps(moveX_,moveZ_,couchX,couchZ,.65f,1.20f);
    if (hitsDesk || hitsChair || hitsCouch) { moveX_ = oldMoveX; moveZ_ = oldMoveZ; }
    snapCooldown_=std::max(0.0f,snapCooldown_-dt);
    if(settings_.smoothTurn){
        const bool turning = std::fabs(rightStickX) > 0.18f;
        if (turning && !previousSmoothTurning_) rebaseYawPivot(headPose);
        virtualYaw_+=std::clamp(rightStickX,-1.0f,1.0f)*std::clamp(settings_.turnSpeed,0.2f,3.5f)*dt;
        previousSmoothTurning_ = turning;
        previousTurnRight_ = false;
        previousTurnLeft_ = false;
    } else {
        previousSmoothTurning_ = false;
        const bool right=rightStickX>0.7f,left=rightStickX < -0.7f;
        if(snapCooldown_<=0.0f && right && !previousTurnRight_){
            rebaseYawPivot(headPose);
            virtualYaw_-=settings_.snapTurnDegrees*kPi/180.0f;
            snapCooldown_=0.24f;
        } else if(snapCooldown_<=0.0f && left && !previousTurnLeft_){
            rebaseYawPivot(headPose);
            virtualYaw_+=settings_.snapTurnDegrees*kPi/180.0f;
            snapCooldown_=0.24f;
        }
        previousTurnRight_=right;previousTurnLeft_=left;
    }
}


void RoomRenderer::rebaseYawPivot(const XrPosef& headPose) {
    // Pivot around the user's current virtual position, including joystick locomotion.
    yawPivotX_ = headPose.position.x + moveX_ + worldOffsetX_;
    yawPivotY_ = headPose.position.y;
    yawPivotZ_ = headPose.position.z + moveZ_ + worldOffsetZ_;
    yawPivotInitialized_ = true;
}

void RoomRenderer::renderEye(int eyeIndex,int width,int height,const XrView& view,GLuint videoTexture,bool hasVideo) {
    if(!initialized_)return;
    float halfW=3.0f,halfD=3.8f;
    if(settings_.roomSize=="large"){halfW=6.0f;halfD=7.5f;}else if(settings_.roomSize=="medium"){halfW=4.2f;halfD=5.4f;}
    (void)eyeIndex; (void)width; (void)height;
    const float floorY=settings_.floorOffsetY, ceilingY=floorY+4.4f, backZ=-halfD, frontZ=halfD;
    Mat4 projection=makeProjection(view.fov);
    const float sittingDrop = sitting_ ? std::clamp(settings_.sittingHeadDropM, 0.10f, 0.75f) : 0.0f;
    // Keep the actual per-eye OpenXR pose untouched. Virtual yaw and locomotion are
    // represented as a shared transform of the virtual world, not a fake compositor pose.
    const Mat4 physicalView = makeView(view.pose, 0.0f, 0.0f, 0.0f, 0.0f);
    // Apply virtual room rotation about its pivot first, then apply locomotion in
    // room/world space. Translation stays outside the rotation so turning cannot
    // rotate or reverse the direction vector used for joystick movement.
    const Mat4 inverseVirtualRotation = mul(
        translation(yawPivotX_, yawPivotY_, yawPivotZ_),
        mul(rotateY(-virtualYaw_), translation(-yawPivotX_, -yawPivotY_, -yawPivotZ_)));
    const Mat4 inverseVirtualTranslation =
        translation(-(moveX_ + worldOffsetX_), sittingDrop, -(moveZ_ + worldOffsetZ_));
    const Mat4 inverseVirtualWorld = mul(inverseVirtualTranslation, inverseVirtualRotation);
    Mat4 camera = mul(physicalView, inverseVirtualWorld);
    Mat4 pv=mul(projection,camera);
    const auto& env=environmentTextures_[activeEnvironmentIndex_];
    std::array<float,4> wall = rgb(.14f,.17f,.25f);
    std::array<float,4> floor = rgb(.09f,.12f,.18f);
    std::array<float,4> wood = rgb(.22f,.18f,.22f);
    std::array<float,4> sofa = rgb(.19f,.24f,.33f);
    switch (activeEnvironmentIndex_) {
    case 0: wall=rgb(.70f,.63f,.67f); floor=rgb(.44f,.37f,.41f); wood=rgb(.32f,.20f,.21f); sofa=rgb(.63f,.38f,.48f); break;
    case 1: wall=rgb(.10f,.13f,.20f); floor=rgb(.10f,.14f,.19f); wood=rgb(.12f,.14f,.17f); sofa=rgb(.11f,.20f,.28f); break;
    case 2: wall=rgb(.20f,.15f,.12f); floor=rgb(.17f,.12f,.09f); wood=rgb(.30f,.18f,.09f); sofa=rgb(.45f,.25f,.09f); break;
    case 3: wall=rgb(.07f,.10f,.18f); floor=rgb(.035f,.055f,.10f); wood=rgb(.11f,.10f,.21f); sofa=rgb(.18f,.22f,.36f); break;
    case 4: case 17: case 18: case 19: case 24:
        wall=rgb(.09f,.055f,.16f); floor=rgb(.035f,.025f,.075f); wood=rgb(.12f,.045f,.20f); sofa=rgb(.18f,.08f,.23f); break;
    case 5: case 8: case 14: case 16: case 25:
        wall=rgb(.34f,.21f,.26f); floor=rgb(.27f,.17f,.16f); wood=rgb(.33f,.20f,.12f); sofa=rgb(.49f,.24f,.25f); break;
    case 6: case 11: case 20: case 27:
        wall=rgb(.12f,.22f,.31f); floor=rgb(.12f,.19f,.25f); wood=rgb(.19f,.26f,.30f); sofa=rgb(.25f,.35f,.42f); break;
    case 7: case 21: case 26:
        wall=rgb(.07f,.19f,.16f); floor=rgb(.045f,.12f,.10f); wood=rgb(.13f,.20f,.13f); sofa=rgb(.12f,.26f,.18f); break;
    case 9: case 13: case 15:
        wall=rgb(.055f,.20f,.26f); floor=rgb(.03f,.12f,.17f); wood=rgb(.10f,.23f,.23f); sofa=rgb(.10f,.31f,.34f); break;
    case 10: case 23:
        wall=rgb(.06f,.06f,.17f); floor=rgb(.025f,.025f,.09f); wood=rgb(.13f,.09f,.24f); sofa=rgb(.13f,.12f,.30f); break;
    case 12: case 22:
        wall=rgb(.22f,.10f,.09f); floor=rgb(.12f,.06f,.065f); wood=rgb(.22f,.12f,.085f); sofa=rgb(.28f,.13f,.11f); break;
    default: break;
    }
    if (activeEnvironmentIndex_ >= 28) {
        switch ((activeEnvironmentIndex_ - 28) / 8) {
        case 0: wall=rgb(.10f,.12f,.22f); floor=rgb(.07f,.08f,.15f); wood=rgb(.10f,.11f,.22f); sofa=rgb(.18f,.11f,.30f); break;
        case 1: wall=rgb(.10f,.20f,.15f); floor=rgb(.06f,.13f,.09f); wood=rgb(.13f,.21f,.13f); sofa=rgb(.14f,.30f,.19f); break;
        case 2: wall=rgb(.06f,.19f,.24f); floor=rgb(.03f,.11f,.16f); wood=rgb(.08f,.19f,.23f); sofa=rgb(.08f,.29f,.34f); break;
        case 3: wall=rgb(.12f,.20f,.29f); floor=rgb(.09f,.15f,.22f); wood=rgb(.16f,.25f,.31f); sofa=rgb(.21f,.34f,.43f); break;
        case 4: wall=rgb(.08f,.07f,.20f); floor=rgb(.035f,.03f,.11f); wood=rgb(.12f,.07f,.25f); sofa=rgb(.19f,.11f,.34f); break;
        case 5: wall=rgb(.25f,.13f,.10f); floor=rgb(.14f,.07f,.055f); wood=rgb(.24f,.12f,.07f); sofa=rgb(.35f,.16f,.11f); break;
        case 6: wall=rgb(.24f,.18f,.25f); floor=rgb(.16f,.12f,.21f); wood=rgb(.28f,.19f,.22f); sofa=rgb(.42f,.24f,.34f); break;
        default: wall=rgb(.11f,.08f,.25f); floor=rgb(.045f,.04f,.15f); wood=rgb(.15f,.09f,.28f); sofa=rgb(.23f,.12f,.37f); break;
        }
    }
    if (activeEnvironmentIndex_ >= 88) {
        const auto palette = extraEnvironmentPalette(activeEnvironmentIndex_ - 28);
        wall = rgb(palette.skyHorizon[0] / 460.0f, palette.skyHorizon[1] / 460.0f, palette.skyHorizon[2] / 460.0f);
        floor = rgb(palette.ground[0] / 510.0f, palette.ground[1] / 510.0f, palette.ground[2] / 510.0f);
        wood = rgb(palette.skyTop[0] / 420.0f, palette.skyTop[1] / 420.0f, palette.skyTop[2] / 420.0f);
        sofa = rgb(palette.skyHorizon[0] / 400.0f, palette.skyHorizon[1] / 400.0f, palette.skyHorizon[2] / 400.0f);
    }
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
    drawEnvironmentWindow(pv.data(),halfW,floorY,backZ,env,std::min(halfW*1.35f,settings_.roomSize=="small"?3.4f:6.2f),activeEnvironmentIndex_==1?2.55f:activeEnvironmentIndex_>=3?2.15f:1.8f);

    const float deskZ=-halfD+1.35f;

    const auto accentBlue = rgb(.16f,.58f,1.0f);
    const auto accentViolet = rgb(.48f,.25f,1.0f);
    const auto trimDark = rgb(.035f,.045f,.075f);
    const auto silver = rgb(.55f,.61f,.70f);
    const float trimY = floorY + 3.72f;
    drawBox(pv.data(),0,trimY,0,halfW*1.62f,.035f,.045f,trimDark);
    drawBox(pv.data(),0,trimY-.035f,0,halfW*1.48f,.018f,.028f,accentViolet);
    drawBox(pv.data(),-halfW+.12f,trimY-.08f,0,.035f,.035f,halfD*1.65f,accentBlue);
    drawBox(pv.data(), halfW-.12f,trimY-.08f,0,.035f,.035f,halfD*1.65f,accentViolet);

    // Acoustic wall panels with inset highlights; keep the selected environment as the window view.
    const float panelY=floorY+2.48f, panelZ=backZ+.16f;
    for(int side : {-1,1}) {
        const float panelX=side*std::min(halfW*.72f,2.65f);
        drawBox(pv.data(),panelX,panelY,panelZ,.42f,1.08f,.06f,trimDark);
        drawBox(pv.data(),panelX,panelY,panelZ+.035f,.026f,.88f,.018f,side<0?accentBlue:accentViolet);
        drawBox(pv.data(),panelX+side*.12f,panelY,panelZ+.035f,.018f,.68f,.018f,rgb(.12f,.17f,.28f));
    }

    // Architectural finish pass: layered skirting, wall reveals, ceiling coffers,
    // and repeated floor inlays give the room real scale and construction detail.
    const auto wallTrim=rgb(.12f,.14f,.20f);
    const auto wallHighlight=rgb(.22f,.25f,.33f);
    const auto floorInlay=rgb(.10f,.12f,.17f);
    const float roomHeight=ceilingY-floorY;
    for(int side : {-1,1}) {
        const float wx=side*(halfW-.105f);
        drawBox(pv.data(),wx,floorY+.075f,0,.045f,.15f,halfD*1.92f,wallTrim);
        drawBox(pv.data(),wx,floorY+.158f,0,.012f,.012f,halfD*1.92f,wallHighlight);
        drawBox(pv.data(),wx,ceilingY-.13f,0,.035f,.16f,halfD*1.90f,wallTrim);
        drawBox(pv.data(),wx,ceilingY-.215f,0,.012f,.010f,halfD*1.90f,wallHighlight);
        for(int bay=0;bay<5;++bay) {
            const float z=-halfD*.78f+bay*(halfD*1.56f/4.0f);
            drawBox(pv.data(),wx-side*.027f,floorY+roomHeight*.48f,z,
                    .012f,roomHeight*.58f,.012f,wallHighlight);
            drawBox(pv.data(),wx-side*.028f,floorY+roomHeight*.48f,z+.035f,
                    .008f,roomHeight*.54f,.006f,wallTrim);
        }
    }
    // Rear-wall picture-frame moulding and a broad lower wainscot band.
    drawBox(pv.data(),0,floorY+.56f,backZ+.018f,halfW*1.88f,.025f,.035f,wallTrim);
    drawBox(pv.data(),0,floorY+.59f,backZ+.018f,halfW*1.88f,.009f,.038f,wallHighlight);
    drawBox(pv.data(),0,ceilingY-.20f,backZ+.018f,halfW*1.88f,.035f,.045f,wallTrim);
    for(int panel=0;panel<7;++panel) {
        const float px=-halfW*.82f+panel*(halfW*1.64f/6.0f);
        drawBox(pv.data(),px,floorY+.31f,backZ+.012f,.018f,.43f,.025f,wallHighlight);
    }
    // Recessed ceiling coffers and warm downlight apertures.
    for(int side : {-1,1}) {
        drawBox(pv.data(),side*halfW*.46f,ceilingY-.035f,0,.018f,.025f,halfD*1.72f,wallTrim);
        drawBox(pv.data(),side*halfW*.46f,ceilingY-.052f,0,.010f,.008f,halfD*1.66f,wallHighlight);
    }
    for(int row=0;row<2;++row) for(int col=0;col<3;++col) {
        const float lx=(col-1)*halfW*.62f;
        const float lz=(row==0?-1.0f:1.0f)*halfD*.42f;
        drawBox(pv.data(),lx,ceilingY-.012f,lz,.34f,.012f,.12f,trimDark);
        drawBox(pv.data(),lx,ceilingY-.020f,lz,.25f,.006f,.075f,
                row==0?rgb(.92f,.78f,.60f):rgb(.68f,.78f,1.0f));
    }
    // Individual floorboard seams, with alternating lengths and subtle tonal variation.
    for(int row=0;row<11;++row) {
        const float z=-halfD*.88f+row*(halfD*1.76f/10.0f);
        drawBox(pv.data(),0,floorY+.006f,z,halfW*1.94f,.006f,.008f,floorInlay);
        for(int seam=0;seam<4;++seam) {
            const float x=-halfW*.72f+(seam+(row%2)*.5f)*(halfW*1.44f/4.0f);
            drawBox(pv.data(),x,floorY+.007f,z+.04f,.006f,.006f,.075f,floorInlay);
        }
    }

    // Gaming desk occupies the back-right corner, leaving the center of the room open.
    const float deskX=halfW-1.48f;
    const float deskWidth=std::min(2.65f,halfW*1.02f);
    const float deskFrontZ=deskZ+.45f;
    const float deskTopY=floorY+.78f;
    drawBox(pv.data(),deskX,deskTopY,deskZ,deskWidth,.085f,.88f,wood);
    drawBox(pv.data(),deskX-deskWidth*.42f,floorY+.39f,deskZ,.075f,.70f,.78f,trimDark);
    drawBox(pv.data(),deskX+deskWidth*.42f,floorY+.39f,deskZ,.075f,.70f,.78f,trimDark);
    drawBox(pv.data(),deskX,deskTopY+.055f,deskZ,deskWidth-.08f,.018f,.80f,rgb(.10f,.12f,.17f));
    // Finished desk apron, inset drawer fronts, pulls, and cable tray.
    drawBox(pv.data(),deskX,deskTopY-.105f,deskZ-.365f,deskWidth*.82f,.12f,.035f,trimDark);
    for(int drawer=0;drawer<3;++drawer) {
        const float dx=deskX+(drawer-1)*.31f;
        drawBox(pv.data(),dx,deskTopY-.105f,deskZ-.386f,.285f,.082f,.010f,wood);
        drawBox(pv.data(),dx,deskTopY-.105f,deskZ-.394f,.065f,.006f,.008f,silver);
    }
    drawBox(pv.data(),deskX,deskTopY-.19f,deskZ+.34f,deskWidth*.72f,.035f,.12f,trimDark);
    drawBox(pv.data(),deskX,deskTopY-.215f,deskZ+.34f,deskWidth*.64f,.012f,.08f,wallHighlight);
    // Cable raceway drops discreetly behind the right pedestal.
    drawBox(pv.data(),deskX+deskWidth*.39f,deskTopY-.32f,deskZ+.34f,.045f,.24f,.055f,trimDark);
    drawBox(pv.data(),deskX+deskWidth*.39f,deskTopY-.45f,deskZ+.31f,.075f,.025f,.09f,silver);

    // Slim under-desk light, softly reflected by the floor.
    drawBox(pv.data(),deskX,deskTopY-.10f,deskFrontZ,.95f,.018f,.025f,accentBlue);

    // Premium curved ultrawide display: continuous curved shell, satin rim, inset live panel and twin-foot stand.
    const int monitorCount=std::clamp(settings_.monitorCount,1,3);
    const float monitorW=monitorCount==1?1.30f:monitorCount==2?.82f:.64f;
    const float monitorH=monitorW*9.0f/16.0f;
    const float monitorX=deskX-.36f;
    const float monitorY=floorY+1.43f;
    const float monitorZ=deskZ-.34f;
    const float deskSurface=floorY+.83f;
    const float standHeight=std::max(.08f,monitorY-monitorH*.5f-deskSurface);
    const float monitorGap=.055f;
    const float curve=.082f;
    for(int i=0;i<monitorCount;++i) {
        const float x=monitorX+(i-(monitorCount-1)*.5f)*(monitorW+monitorGap);
        const bool streamOn=settings_.screenDestination=="monitor"||settings_.screenDestination=="both";
        const GLuint tex=(streamOn&&hasVideo)?videoTexture:videoFallbackTexture_;
        // A real layered housing: rear cover, deep sidewalls, metallic front lip, then the glass.
        const float shellW=monitorW+.090f;
        const float shellH=monitorH+.090f;
        const float shellCurve=curve+.008f;
        drawCurvedPanel(pv.data(),x,monitorY,monitorZ-.020f,shellW,shellH,shellCurve,0,trimDark,32);
        drawCurvedPanel(pv.data(),x,monitorY,monitorZ+.018f,shellW,shellH,shellCurve,0,trimDark,32);
        drawCurvedPanel(pv.data(),x,monitorY,monitorZ+.032f,monitorW+.040f,monitorH+.040f,curve*.88f,0,silver,32);
        drawCurvedPanel(pv.data(),x,monitorY,monitorZ+.044f,monitorW,monitorH,curve*.76f,tex,rgb(1,1,1),32);
        // Segmented edge rails follow the curvature and bridge the front and rear covers,
        // giving the panel visible thickness instead of the appearance of three floating sheets.
        constexpr int edgeSegments=32;
        for(int segment=0;segment<edgeSegments;++segment) {
            const float u=(static_cast<float>(segment)+.5f)/static_cast<float>(edgeSegments);
            const float nx=2.0f*u-1.0f;
            const float px=x+nx*shellW*.5f;
            const float arc=shellCurve*(1.0f-nx*nx);
            const float frontZ=monitorZ+.018f+arc;
            const float railZ=frontZ-.020f;
            const float railW=shellW/static_cast<float>(edgeSegments)+.003f;
            const float yaw=std::atan(4.0f*shellCurve*nx/shellW);
            drawBox(pv.data(),px,monitorY+shellH*.5f-.004f,railZ,railW,.028f,.042f,trimDark,yaw);
            drawBox(pv.data(),px,monitorY-shellH*.5f+.004f,railZ,railW,.028f,.042f,trimDark,yaw);
            // Fine satin highlights on the upper edge catch the room's ambient colors.
            drawBox(pv.data(),px,monitorY+shellH*.5f-.003f,frontZ+.001f,railW,.004f,.004f,silver,yaw);
        }
        for(int side : {-1,1}) {
            const float edgeX=x+side*shellW*.5f;
            drawBox(pv.data(),edgeX,monitorY,monitorZ+.018f-.020f,.028f,shellH-.035f,.042f,trimDark);
            drawBox(pv.data(),edgeX-side*.002f,monitorY,monitorZ+.018f,.006f,shellH-.055f,.006f,silver);
        }
        // A restrained lower chin and tiny centered status light sit beneath the active image.
        drawBox(pv.data(),x,monitorY-monitorH*.5f-.014f,monitorZ+.070f,monitorW*.42f,.014f,.014f,trimDark);
        drawBox(pv.data(),x,monitorY-monitorH*.5f-.016f,monitorZ+.079f,.055f,.005f,.006f,accentBlue);
        // Twin metal uprights attach through a centered VESA-style mounting plate.
        drawBox(pv.data(),x,monitorY-monitorH*.5f+.055f,monitorZ-.047f,.25f,.13f,.028f,trimDark);
        drawBox(pv.data(),x,monitorY-monitorH*.5f+.055f,monitorZ-.031f,.19f,.075f,.008f,silver);
        // Rear service spine, ventilation slots, and a small illuminated model mark make
        // the monitor feel finished when seen from the side or while walking around it.
        const float rearSpineY=monitorY+monitorH*.12f;
        drawBox(pv.data(),x,rearSpineY,monitorZ+.057f,.25f,.29f,.024f,trimDark);
        drawBox(pv.data(),x,rearSpineY+.115f,monitorZ+.043f,.105f,.006f,.005f,accentBlue);
        for(int vent=0;vent<5;++vent) {
            const float ventY=rearSpineY+.065f-static_cast<float>(vent)*.027f;
            drawBox(pv.data(),x,ventY,monitorZ+.043f,.105f,.007f,.005f,rgb(.24f,.29f,.38f));
        }
        drawBox(pv.data(),x,rearSpineY-.105f,monitorZ+.043f,.045f,.018f,.005f,silver);
        const float legOffset=monitorW*.205f;
        for(int side=-1;side<=1;side+=2) {
            const float legX=x+side*legOffset;
            // Slightly inset uprights read as structural supports, not bars glued to the screen.
            drawBox(pv.data(),legX,deskSurface+standHeight*.5f,monitorZ-.026f,.029f,standHeight,.036f,silver);
            drawBox(pv.data(),legX,deskSurface+.018f,monitorZ+.075f,.078f,.030f,.34f,trimDark,side*.10f);
            drawBox(pv.data(),legX,deskSurface+.035f,monitorZ+.075f,.044f,.008f,.29f,silver,side*.10f);
            // Small blue inlays are recessed into the two low-profile feet.
            drawBox(pv.data(),legX,deskSurface+.040f,monitorZ+.025f,.026f,.004f,.045f,accentBlue,side*.10f);
            // Sculpted toe caps soften the otherwise machined twin-foot profile.
            drawSphere(pv.data(),legX,deskSurface+.018f,monitorZ+.235f,.039f,.015f,.030f,trimDark);
            drawSphere(pv.data(),legX,deskSurface+.018f,monitorZ-.085f,.039f,.015f,.030f,trimDark);
        }

        // Front-facing hardware details: a centered camera window, paired microphone
        // apertures, a discrete power key and fine speaker perforations in the chin.
        const float screenTop=monitorY+monitorH*.5f;
        const float screenBottom=monitorY-monitorH*.5f;
        drawSphere(pv.data(),x,screenTop+.014f,monitorZ+.080f,.012f,.006f,.004f,trimDark);
        drawSphere(pv.data(),x-.024f,screenTop+.014f,monitorZ+.080f,.0035f,.0035f,.0025f,silver);
        drawSphere(pv.data(),x+.024f,screenTop+.014f,monitorZ+.080f,.0035f,.0035f,.0025f,silver);
        drawSphere(pv.data(),x+monitorW*.455f,screenBottom+.006f,monitorZ+.079f,.010f,.010f,.004f,trimDark);
        drawSphere(pv.data(),x+monitorW*.455f,screenBottom+.006f,monitorZ+.083f,.004f,.004f,.002f,accentBlue);
        for(int grille=0;grille<2;++grille) {
            const float side=grille==0?-1.0f:1.0f;
            for(int hole=0;hole<9;++hole) {
                const float hx=x+side*(monitorW*.23f+hole*.018f);
                drawSphere(pv.data(),hx,screenBottom-.014f,monitorZ+.079f,
                           .0024f,.0024f,.0018f,rgb(.20f,.23f,.29f));
            }
        }

        // Two banks of individually modeled rear cooling louvers flank the spine.
        // The repeated slats create readable depth instead of a single painted vent stripe.
        for(int bank=-1;bank<=1;bank+=2) {
            for(int row=0;row<4;++row) {
                const float ventY=rearSpineY+.075f-row*.032f;
                for(int slot=0;slot<7;++slot) {
                    const float ventX=x+bank*(.17f+slot*.021f);
                    drawBox(pv.data(),ventX,ventY,monitorZ+.052f,.010f,.0045f,.004f,
                            rgb(.18f,.22f,.29f),bank*.035f);
                }
            }
        }
        // Recessed connector bay and individual I/O sockets on the lower rear edge.
        drawBox(pv.data(),x+shellW*.5f-.008f,monitorY-monitorH*.18f,monitorZ-.004f,
                .012f,.105f,.030f,trimDark);
        for(int port=0;port<3;++port) {
            const float portY=monitorY-monitorH*.18f+.032f-port*.030f;
            drawBox(pv.data(),x+shellW*.5f-.001f,portY,monitorZ-.004f,
                    .004f,.012f,.020f,rgb(.10f,.13f,.19f));
            drawBox(pv.data(),x+shellW*.5f+.001f,portY,monitorZ-.004f,
                    .002f,.004f,.010f,silver);
        }
        // Four metallic fastener heads make the VESA mount read as assembled hardware.
        for(int sx : {-1,1}) for(int sy : {-1,1}) {
            drawSphere(pv.data(),x+sx*.092f,monitorY-monitorH*.5f+.055f+sy*.045f,
                       monitorZ-.030f,.006f,.006f,.003f,silver);
        }
        // A fine perimeter seam on the rear cover and a small raised model badge.
        drawBox(pv.data(),x,monitorY+shellH*.5f-.018f,monitorZ+.040f,
                shellW*.72f,.003f,.004f,rgb(.11f,.14f,.20f));
        drawBox(pv.data(),x,monitorY-monitorH*.5f+.12f,monitorZ+.044f,
                .082f,.020f,.006f,trimDark);
        drawSphere(pv.data(),x,monitorY-monitorH*.5f+.12f,monitorZ+.049f,
                   .006f,.006f,.002f,accentBlue);
    }

    // A compact white gaming PC with a glass side panel, ventilation detail and three illuminated fans.
    const float pcX=deskX+deskWidth*.31f;
    const float pcY=deskSurface+.34f;
    const float pcZ=deskZ+.12f;
    const auto pcWhite=rgb(.88f,.91f,.96f);
    const auto pcEdge=rgb(.66f,.72f,.80f);
    drawBox(pv.data(),pcX,pcY,pcZ,.43f,.66f,.57f,pcWhite);
    drawBox(pv.data(),pcX,pcY,pcZ+.292f,.37f,.60f,.012f,rgb(.10f,.13f,.20f));
    drawBox(pv.data(),pcX-.16f,pcY,pcZ+.302f,.045f,.55f,.008f,rgb(.20f,.24f,.33f));
    drawBox(pv.data(),pcX+.16f,pcY,pcZ+.302f,.045f,.55f,.008f,rgb(.20f,.24f,.33f));
    for(int fan=0;fan<3;++fan) {
        const float fy=pcY+.20f-fan*.20f;
        drawSphere(pv.data(),pcX,fy,pcZ+.315f,.060f,.060f,.012f,fan==1?accentViolet:accentBlue);
        drawSphere(pv.data(),pcX,fy,pcZ+.329f,.027f,.027f,.009f,rgb(.88f,.92f,1.0f));
    }
    // Individual front I/O sockets, feet, and side-panel fasteners.
    for(int port=0;port<2;++port) {
        drawBox(pv.data(),pcX-.095f+port*.065f,pcY-.285f,pcZ+.322f,.032f,.012f,.006f,silver);
    }
    for(int foot : {-1,1}) {
        drawBox(pv.data(),pcX-.15f,pcY-.345f,pcZ+foot*.21f,.09f,.035f,.10f,trimDark);
        drawBox(pv.data(),pcX+.15f,pcY-.345f,pcZ+foot*.21f,.09f,.035f,.10f,trimDark);
    }
    for(int fastener : {-1,1}) {
        drawSphere(pv.data(),pcX+fastener*.17f,pcY+.27f,pcZ+.310f,.008f,.008f,.004f,silver);
    }
    // Discreet top vent and power button.
    for(int vent=0;vent<5;++vent)
        drawBox(pv.data(),pcX-.12f+vent*.06f,pcY+.337f,pcZ,.035f,.006f,.24f,pcEdge);
    drawSphere(pv.data(),pcX+.13f,pcY+.337f,pcZ+.18f,.018f,.012f,.018f,accentBlue);

    // TV on the left wall, facing into the room; the couch across from it faces the TV.
    if(settings_.showTv) {
        const float tvX=-halfW+.075f, tvY=floorY+2.02f, tvZ=.05f;
        const float tvW=std::min(2.0f,halfD*.58f), tvH=tvW*9.0f/16.0f;
        const bool tvStream=settings_.screenDestination=="tv"||settings_.screenDestination=="both";
        const GLuint tvTex=(tvStream&&hasVideo)?videoTexture:env;
        drawBox(pv.data(),tvX,tvY,tvZ,.095f,tvH+.12f,tvW+.12f,trimDark);
        drawBox(pv.data(),tvX+.05f,tvY,tvZ,.018f,tvH+.035f,tvW+.035f,silver);
        drawPlane(pv.data(),tvX+.062f,tvY,tvZ,tvW,tvH,tvTex,rgb(1,1,1),kPi*.5f);
        drawBox(pv.data(),tvX+.08f,tvY-tvH*.5f-.08f,tvZ,.025f,.018f,.28f,accentBlue);
    }

    // Couch moved directly opposite the wall TV. Soft, rounded cushions replace the old blocky sofa.
    if(settings_.showCouch) {
        const float couchX=halfW-1.25f, couchZ=.05f, couchYaw=-kPi*.5f;
        const float cx=couchX;
        drawBox(pv.data(),cx,floorY+.38f,couchZ, .86f,.24f,1.92f,trimDark,couchYaw);
        drawSphere(pv.data(),cx,floorY+.53f,couchZ,.48f,.24f,.82f,sofa);
        // Raised back is on the +X side, so the sitting position faces the TV on the -X wall.
        drawSphere(pv.data(),cx+.22f,floorY+.68f,couchZ-.48f,.25f,.28f,.39f,sofa);
        drawSphere(pv.data(),cx+.22f,floorY+.68f,couchZ+.48f,.25f,.28f,.39f,sofa);
        drawSphere(pv.data(),cx-.12f,floorY+.75f,couchZ-.23f,.22f,.15f,.35f,rgb(sofa[0]*1.18f,sofa[1]*1.18f,sofa[2]*1.18f));
        drawSphere(pv.data(),cx-.12f,floorY+.75f,couchZ+.23f,.22f,.15f,.35f,rgb(sofa[0]*1.18f,sofa[1]*1.18f,sofa[2]*1.18f));
        // Separate seat and back cushions with stitched piping and panel seams.
        drawSphere(pv.data(),cx-.13f,floorY+.49f,couchZ-.28f,.30f,.10f,.34f,rgb(sofa[0]*1.12f,sofa[1]*1.12f,sofa[2]*1.12f));
        drawSphere(pv.data(),cx-.13f,floorY+.49f,couchZ+.28f,.30f,.10f,.34f,rgb(sofa[0]*1.12f,sofa[1]*1.12f,sofa[2]*1.12f));
        drawBox(pv.data(),cx-.18f,floorY+.51f,couchZ,.012f,.006f,.78f,wallHighlight,couchYaw);
        // Rounded arm pads and short dark feet.
        drawSphere(pv.data(),cx,floorY+.61f,couchZ-.88f,.48f,.27f,.16f,sofa);
        drawSphere(pv.data(),cx,floorY+.61f,couchZ+.88f,.48f,.27f,.16f,sofa);
        for(int side : {-1,1}) for(int end : {-1,1})
            drawBox(pv.data(),cx+side*.29f,floorY+.15f,couchZ+end*.70f,.07f,.25f,.07f,trimDark,couchYaw);
    }

    // Ergonomic gaming chair with rounded headrest, cushioned seat and five-star base.
    if(settings_.showChair) {
        const float chairX=deskX-.25f, chairZ=deskZ+1.20f;
        drawSphere(pv.data(),chairX,floorY+.62f,chairZ,.36f,.13f,.34f,sofa);
        drawSphere(pv.data(),chairX,floorY+1.00f,chairZ-.22f,.34f,.44f,.13f,sofa);
        drawSphere(pv.data(),chairX,floorY+1.34f,chairZ-.24f,.20f,.18f,.12f,sofa);
        drawBox(pv.data(),chairX,floorY+.27f,chairZ,.07f,.38f,.07f,silver);
        for(int i=0;i<5;++i) {
            const float a=2*kPi*i/5;
            drawBox(pv.data(),chairX+std::cos(a)*.30f,floorY+.07f,chairZ+std::sin(a)*.28f,.32f,.045f,.055f,trimDark,a);
        }
        drawSphere(pv.data(),chairX-.25f,floorY+.57f,chairZ,.10f,.06f,.26f,trimDark);
        drawSphere(pv.data(),chairX+.25f,floorY+.57f,chairZ,.10f,.06f,.26f,trimDark);
        // Padded arm caps, lumbar insert, and segmented backrest stitching.
        drawSphere(pv.data(),chairX,floorY+.91f,chairZ-.105f,.24f,.20f,.035f,rgb(sofa[0]*1.18f,sofa[1]*1.18f,sofa[2]*1.18f));
        drawSphere(pv.data(),chairX-.25f,floorY+.63f,chairZ,.13f,.055f,.25f,sofa);
        drawSphere(pv.data(),chairX+.25f,floorY+.63f,chairZ,.13f,.055f,.25f,sofa);
        for(int seam=0;seam<3;++seam)
            drawBox(pv.data(),chairX,floorY+.82f+seam*.09f,chairZ-.115f,.34f,.004f,.008f,wallHighlight);
    }

    // Warm practical lamp and a restrained light accent near the PC setup.
    drawBox(pv.data(),-halfW+.30f,floorY+1.35f,-1.45f,.09f,.90f,.09f,wood);
    const auto lampGlow=activeEnvironmentIndex_==0?rgb(.98f,.72f,.82f):
        (activeEnvironmentIndex_==3||activeEnvironmentIndex_==26)?rgb(.30f,1.0f,.70f):
        (activeEnvironmentIndex_==4||activeEnvironmentIndex_==10||activeEnvironmentIndex_==24)?rgb(.25f,.72f,1.0f):
        (activeEnvironmentIndex_==16||activeEnvironmentIndex_==19||activeEnvironmentIndex_==25)?rgb(1.0f,.53f,.82f):rgb(1.0f,.63f,.30f);
    drawSphere(pv.data(),-halfW+.30f,floorY+1.92f,-1.45f,.20f,.15f,.20f,lampGlow);
    if(settings_.comfortVignette && vignetteProgram_){
        glDisable(GL_DEPTH_TEST);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(vignetteProgram_);glBindVertexArray(planeTexture_);glDrawArrays(GL_TRIANGLES,0,6);glBindVertexArray(0);
        glUseProgram(0);glDisable(GL_BLEND);glEnable(GL_DEPTH_TEST);
    }
    // Monitor-anchored menu: world coordinates place it on the virtual display, not in the user's face.
    if (menuVisible_) {
        const float deskZ = -halfD + 1.35f;
        const float deskX = halfW - 1.48f;
        const float monitorX = deskX - 0.36f;
        const float monitorY = floorY + 1.43f;
        const float monitorZ = deskZ - 0.34f;
        const float menuZ = monitorZ + 0.132f;
        const auto panel = rgb(0.025f, 0.032f, 0.065f, 0.98f);
        const auto row = rgb(0.075f, 0.090f, 0.145f, 0.98f);
        const auto selected = rgb(0.10f, 0.22f, 0.39f, 1.0f);
        const auto blue = rgb(0.16f, 0.58f, 1.0f, 1.0f);
        const auto violet = rgb(0.48f, 0.25f, 1.0f, 1.0f);
        const auto text = rgb(0.88f, 0.92f, 1.0f, 1.0f);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(program_);
        drawBox(pv.data(), monitorX, monitorY, menuZ, 1.02f, 0.60f, 0.008f, panel);
        drawBox(pv.data(), monitorX, monitorY + 0.278f, menuZ + 0.006f, 0.96f, 0.012f, 0.006f, blue);
        drawBox(pv.data(), monitorX - 0.43f, monitorY + 0.225f, menuZ + 0.008f, 0.035f, 0.035f, 0.006f, violet);
        drawText(pv.data(), "QUESTBRIDGE", monitorX - 0.37f, monitorY + 0.218f, menuZ + 0.012f, 0.0060f, text);
        drawText(pv.data(), kEnvironmentNames[activeEnvironmentIndex_], monitorX + 0.08f, monitorY + 0.218f,
                 menuZ + 0.012f, 0.0035f, blue);
        const char* labels[5] = {"ENVIRONMENT", "DISPLAY MODE", "TV VISIBILITY", "COUCH VISIBILITY", "CLOSE MENU"};
        for (int i = 0; i < 5; ++i) {
            const float y = monitorY + 0.135f - static_cast<float>(i) * 0.091f;
            drawBox(pv.data(), monitorX, y - 0.015f, menuZ + 0.006f, 0.94f, 0.073f, 0.006f,
                    i == menuSelection_ ? selected : row);
            drawBox(pv.data(), monitorX - 0.415f, y - 0.015f, menuZ + 0.008f, 0.018f, 0.043f, 0.004f,
                    i == menuSelection_ ? blue : violet);
            drawText(pv.data(), labels[i], monitorX - 0.37f, y + 0.015f, menuZ + 0.012f, 0.0052f, text);
            if (i == 0) drawText(pv.data(), "NEXT", monitorX + 0.24f, y + 0.015f, menuZ + 0.012f, 0.0043f, blue);
            else if (i == 1) {
                const char* mode = settings_.screenDestination == "monitor" ? "MONITOR" :
                    (settings_.screenDestination == "tv" ? "TV" : "BOTH");
                drawText(pv.data(), mode, monitorX + 0.20f, y + 0.015f, menuZ + 0.012f, 0.0043f, blue);
            } else if (i == 2) drawText(pv.data(), settings_.showTv ? "ON" : "OFF", monitorX + 0.32f, y + 0.015f, menuZ + 0.012f, 0.0043f, blue);
            else if (i == 3) drawText(pv.data(), settings_.showCouch ? "ON" : "OFF", monitorX + 0.32f, y + 0.015f, menuZ + 0.012f, 0.0043f, blue);
        }
        glDisable(GL_BLEND);
    }
    glBindTexture(GL_TEXTURE_2D,0);glBindVertexArray(0);glUseProgram(0);glDisable(GL_DEPTH_TEST);
}

void RoomRenderer::release() {
    if(!initialized_&&!program_&&!vao_)return;
    if(program_)glDeleteProgram(program_);if(vignetteProgram_)glDeleteProgram(vignetteProgram_);if(vao_)glDeleteVertexArrays(1,&vao_);if(vbo_)glDeleteBuffers(1,&vbo_);
    if(planeTexture_)glDeleteVertexArrays(1,&planeTexture_);if(cubeTexture_)glDeleteBuffers(1,&cubeTexture_);
    if(textVao_)glDeleteVertexArrays(1,&textVao_);if(textVbo_)glDeleteBuffers(1,&textVbo_);
    if(curvedVao_)glDeleteVertexArrays(1,&curvedVao_);if(curvedVbo_)glDeleteBuffers(1,&curvedVbo_);
    if(sphereVao_)glDeleteVertexArrays(1,&sphereVao_);if(sphereVbo_)glDeleteBuffers(1,&sphereVbo_);
    for(auto t:environmentTextures_)if(t)glDeleteTextures(1,&t);if(videoFallbackTexture_)glDeleteTextures(1,&videoFallbackTexture_);
    program_=vignetteProgram_=vao_=vbo_=planeTexture_=cubeTexture_=videoFallbackTexture_=sphereVao_=sphereVbo_=textVao_=textVbo_=curvedVao_=curvedVbo_=0;sphereVertexCount_=0;environmentTextures_.fill(0);initialized_=false;
}
