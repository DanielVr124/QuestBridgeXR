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

#define STB_TRUETYPE_IMPLEMENTATION
#include "third_party/stb_truetype.h"

#define QBX_LOG_TAG "QuestBridgeRoom"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, QBX_LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, QBX_LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, QBX_LOG_TAG, __VA_ARGS__)

namespace {
constexpr float kPi = 3.14159265358979323846f;
// Matches the imported arc27_curved_monitor.glb screen node's ~0.05 rad pitch.
constexpr float kMonitorScreenPitch = 0.05f;
constexpr int kFontAtlasSize = 512;
constexpr int kFontFirstCharacter = 32;
constexpr int kFontCharacterCount = 96;
GLuint gFontAtlasTexture = 0;
std::array<stbtt_bakedchar, kFontCharacterCount> gFontCharacters{};
bool gFontAtlasReady = false;
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
uniform mat4 uModel;
uniform int uRounded;
uniform vec3 uBevel;
out vec2 vUv;
out vec3 vWorldPosition;
void main(){
  vec3 localPosition=aPosition;
  if(uRounded!=0) {
    vec3 bevel=max(uBevel,vec3(0.0001));
    vec3 inner=clamp(aPosition,vec3(-0.5)+bevel,vec3(0.5)-bevel);
    vec3 delta=aPosition-inner;
    vec3 scaledDelta=delta/bevel;
    float deltaLength=length(scaledDelta);
    if(deltaLength>0.0001) localPosition=inner+(scaledDelta/deltaLength)*bevel;
  }
  vec4 world=uModel*vec4(localPosition,1.0);
  gl_Position = uMvp * vec4(localPosition,1.0);
  vUv=aUv;
  vWorldPosition=world.xyz;
}
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
in vec3 vWorldPosition;
uniform vec4 uColor;
uniform sampler2D uTexture;
uniform sampler2D uMaterialAtlas;
uniform int uHasMaterialAtlas;
uniform sampler2D uNormalAtlas;
uniform int uHasNormalAtlas;
uniform sampler2D uRoughnessAtlas;
uniform int uHasRoughnessAtlas;
uniform int uTextured;
uniform int uMaterial;
uniform vec2 uUvScale;
uniform vec3 uCameraPosition;
uniform float uRoomAmbientIntensity;
uniform vec3 uCeilingLightPosition;
uniform vec3 uCeilingLightColor;
uniform float uCeilingLightIntensity;
uniform vec3 uPracticalLightPosition;
uniform vec3 uPracticalLightColor;
uniform float uPracticalLightIntensity;
uniform vec3 uPracticalLight2Position;
uniform vec3 uPracticalLight2Color;
uniform float uPracticalLight2Intensity;
uniform vec3 uHeadboardGlowPosition;
uniform vec3 uHeadboardGlowColor;
uniform float uHeadboardGlowIntensity;
uniform vec3 uNeonBluePosition;
uniform vec3 uNeonBlueColor;
uniform float uNeonBlueIntensity;
uniform vec3 uNeonVioletPosition;
uniform vec3 uNeonVioletColor;
uniform float uNeonVioletIntensity;
uniform vec3 uNeonPinkPosition;
uniform vec3 uNeonPinkColor;
uniform float uNeonPinkIntensity;
out vec4 outColor;

float hash21(vec2 p) {
  p=fract(p*vec2(123.34,456.21));
  p+=dot(p,p+45.32);
  return fract(p.x*p.y);
}
float noise2(vec2 p) {
  vec2 i=floor(p), f=fract(p);
  f=f*f*(3.0-2.0*f);
  return mix(mix(hash21(i),hash21(i+vec2(1,0)),f.x),
             mix(hash21(i+vec2(0,1)),hash21(i+vec2(1,1)),f.x),f.y);
}
float fbm(vec2 p) {
  float value=0.0, amplitude=0.5;
  for(int i=0;i<4;++i){value+=noise2(p)*amplitude;p*=2.03;amplitude*=0.5;}
  return value;
}
const float kMaterialPi=3.14159265359;
vec3 evaluatePbrLight(vec3 n,vec3 v,vec3 l,vec3 albedo,float roughness,float metallic,vec3 radiance) {
  float nDotL=max(dot(n,l),0.0);
  float nDotV=max(dot(n,v),0.001);
  if(nDotL<=0.0) return vec3(0.0);
  vec3 h=normalize(v+l);
  float nDotH=max(dot(n,h),0.0);
  float vDotH=max(dot(v,h),0.0);
  float alpha=roughness*roughness;
  float alpha2=alpha*alpha;
  float denom=nDotH*nDotH*(alpha2-1.0)+1.0;
  float distribution=alpha2/max(kMaterialPi*denom*denom,0.0001);
  float k=pow(roughness+1.0,2.0)*0.125;
  float geometryV=nDotV/(nDotV*(1.0-k)+k);
  float geometryL=nDotL/(nDotL*(1.0-k)+k);
  vec3 f0=mix(vec3(0.04),albedo,metallic);
  vec3 fresnel=f0+(1.0-f0)*pow(1.0-vDotH,5.0);
  vec3 specular=(distribution*geometryV*geometryL*fresnel)/
                max(4.0*nDotV*nDotL,0.001);
  vec3 diffuseWeight=(1.0-fresnel)*(1.0-metallic);
  vec3 diffuse=diffuseWeight*albedo/kMaterialPi;
  return (diffuse+specular)*radiance*nDotL;
}
void main(){
  vec4 tex = uTextured != 0 ? texture(uTexture, vUv) : vec4(1.0);
  vec4 base = tex * uColor;
  // Emissive shader path for recessed neon strips and illuminated hardware.
  if (uMaterial == 7) { outColor=vec4(min(base.rgb*1.42,vec3(1.0)),base.a); return; }
  if (uTextured != 0 || uMaterial < 0) { outColor=base; return; }

  vec2 uv=vUv*uUvScale;
  vec2 materialAtlasUv=vec2(0.0);
  float roughness=0.72;
  float metallic=0.04;
  float surface=0.0;
  // Keep material response values cheap and deterministic when atlas maps are resident.
  if(uMaterial==1 || uMaterial==6) {
    roughness=0.58;
    if(uHasMaterialAtlas==0) {
      float warp=fbm(uv*vec2(2.2,0.42))*0.20;
      float grain=sin((uv.y+warp)*145.0 + fbm(uv*vec2(4.0,0.8))*5.0);
      float fine=sin((uv.y+warp)*510.0)*0.18;
      surface += grain*0.075 + fine*0.018 + (fbm(uv*vec2(18.0,3.0))-0.5)*0.11;
    }
    if(uMaterial==6) {
      vec2 plank=fract(vec2(uv.x+mod(floor(uv.y),2.0)*0.5,uv.y));
      float seam=min(min(plank.x,1.0-plank.x),min(plank.y,1.0-plank.y));
      surface -= (1.0-smoothstep(0.008,0.035,seam))*0.20;
    }
  } else if(uMaterial==2) {
    roughness=0.92;
    if(uHasMaterialAtlas==0) {
      surface += (fbm(uv*vec2(13.0,11.0))-0.5)*0.075;
      surface += (noise2(uv*vec2(96.0,84.0))-0.5)*0.025;
    }
  } else if(uMaterial==3) {
    roughness=0.94;
    if(uHasMaterialAtlas==0) {
      vec2 weave=fract(uv*vec2(190.0,160.0));
      float thread=max(smoothstep(0.02,0.18,weave.x)*(1.0-smoothstep(0.76,0.98,weave.x)),
                       smoothstep(0.02,0.18,weave.y)*(1.0-smoothstep(0.76,0.98,weave.y)));
      surface += (thread-0.42)*0.045+(fbm(uv*vec2(22.0,22.0))-0.5)*0.05;
    }
  } else if(uMaterial==4) {
    // Satin/brushed metal grain uses the dedicated texture atlas, with this analytic fallback.
    roughness=0.27;
    metallic=0.78;
    if(uHasMaterialAtlas==0) surface += (noise2(vec2(uv.x*7.0,uv.y*260.0))-0.5)*0.035;
  } else if(uMaterial==5) {
    roughness=0.39;
    metallic=0.02;
    if(uHasMaterialAtlas==0) surface += (fbm(uv*vec2(27.0,27.0))-0.5)*0.035;
  } else if(uMaterial==8) {
    // Polished midnight stone: low roughness catches the recessed RGB fixtures as broad highlights.
    roughness=0.19;
    metallic=0.22;
    if(uHasMaterialAtlas==0) {
      float cloud=fbm(uv*vec2(1.7,4.6));
      float fine=fbm(uv*vec2(13.0,2.1));
      float vein=smoothstep(0.69,0.83,fbm(uv*vec2(3.8,8.2)+vec2(4.7,1.3)));
      surface += (cloud-0.48)*0.035 + (fine-0.5)*0.012 - vein*0.045;
    }
  } else if(uHasMaterialAtlas==0) {
    surface += (noise2(uv*140.0)-0.5)*0.025;
  }
  if(uHasMaterialAtlas!=0 && (uMaterial==1 || uMaterial==2 || uMaterial==3 || uMaterial==4 || uMaterial==6)) {
    vec2 atlasOrigin=vec2(0.0,0.0);
    if(uMaterial==2) atlasOrigin=vec2(0.5,0.0);
    else if(uMaterial==3) atlasOrigin=vec2(0.0,0.5);
    else if(uMaterial==4) atlasOrigin=vec2(0.5,0.5);
    // Atlas tiles are inset from their boundaries to prevent mip bleed.
    vec2 atlasTile=fract(uv*0.42);
    vec2 atlasUv=atlasOrigin+vec2(0.012)+atlasTile*0.476;
    materialAtlasUv=atlasUv;
    vec3 albedoTexel=texture(uMaterialAtlas,atlasUv).rgb;
    float texel=dot(albedoTexel,vec3(0.2126,0.7152,0.0722));
    surface += (texel-0.86)*0.55;
    // Subtle material-specific color variation is layered over the room's base palette.
    base.rgb*=mix(vec3(1.0),albedoTexel*1.15,0.38);
    if(uHasRoughnessAtlas!=0) roughness=texture(uRoughnessAtlas,atlasUv).r;
  }
  base.rgb=max(base.rgb*(1.0+surface),vec3(0.0));

  vec3 dpdx=dFdx(vWorldPosition), dpdy=dFdy(vWorldPosition);
  vec3 normal=normalize(cross(dpdx,dpdy));
  if (!gl_FrontFacing) normal=-normal;
  if(uHasNormalAtlas!=0 && uHasMaterialAtlas!=0 &&
     (uMaterial==1 || uMaterial==2 || uMaterial==3 || uMaterial==4 || uMaterial==6)) {
    vec2 duvdx=dFdx(uv), duvdy=dFdy(uv);
    vec3 tangent=normalize(dpdx*duvdy.y-dpdy*duvdx.y);
    vec3 bitangent=normalize(-dpdx*duvdy.x+dpdy*duvdx.x);
    bitangent*=sign(dot(cross(tangent,bitangent),normal));
    vec3 tangentNormal=texture(uNormalAtlas,materialAtlasUv).xyz*2.0-1.0;
    tangentNormal.xy*=0.38;
    tangentNormal=normalize(tangentNormal);
    normal=normalize(mat3(tangent,bitangent,normal)*tangentNormal);
  }
  vec3 viewDir=normalize(uCameraPosition-vWorldPosition);
  roughness=clamp(roughness,0.045,1.0);
  vec3 directional= evaluatePbrLight(normal,viewDir,normalize(vec3(-0.42,0.78,0.46)),
                                     base.rgb,roughness,metallic,vec3(.58,.60,.62)*uRoomAmbientIntensity);
  vec3 ceilingVector=uCeilingLightPosition-vWorldPosition;
  float ceilingDistance=max(length(ceilingVector),0.001);
  vec3 ceilingDirection=ceilingVector/ceilingDistance;
  float ceilingAttenuation=1.0/(1.0+0.12*ceilingDistance+0.055*ceilingDistance*ceilingDistance);
  vec3 ceiling=evaluatePbrLight(normal,viewDir,ceilingDirection,base.rgb,roughness,metallic,
                                uCeilingLightColor*(ceilingAttenuation*uCeilingLightIntensity*.72));
  vec3 practicalVector=uPracticalLightPosition-vWorldPosition;
  float practicalDistance=max(length(practicalVector),0.001);
  vec3 practicalDirection=practicalVector/practicalDistance;
  float practicalAttenuation=1.0/(1.0+0.16*practicalDistance+0.085*practicalDistance*practicalDistance);
  vec3 practical=evaluatePbrLight(normal,viewDir,practicalDirection,base.rgb,roughness,metallic,
                                  uPracticalLightColor*(practicalAttenuation*uPracticalLightIntensity*.58));
  vec3 practical2Vector=uPracticalLight2Position-vWorldPosition;
  float practical2Distance=max(length(practical2Vector),0.001);
  float practical2Attenuation=1.0/(1.0+0.16*practical2Distance+0.085*practical2Distance*practical2Distance);
  vec3 practical2=evaluatePbrLight(normal,viewDir,practical2Vector/practical2Distance,base.rgb,roughness,metallic,
                                   uPracticalLight2Color*(practical2Attenuation*uPracticalLight2Intensity*.58));
  vec3 headboardVector=uHeadboardGlowPosition-vWorldPosition;
  float headboardDistance=max(length(headboardVector),0.001);
  vec3 headboardBounce=evaluatePbrLight(normal,viewDir,headboardVector/headboardDistance,base.rgb,roughness,metallic,
      uHeadboardGlowColor*(uHeadboardGlowIntensity/(1.0+0.18*headboardDistance+0.095*headboardDistance*headboardDistance)));
  vec3 blueVector=uNeonBluePosition-vWorldPosition;
  float blueDistance=max(length(blueVector),0.001);
  vec3 blueBounce=evaluatePbrLight(normal,viewDir,blueVector/blueDistance,base.rgb,roughness,metallic,
      uNeonBlueColor*(uNeonBlueIntensity/(1.0+0.18*blueDistance+0.095*blueDistance*blueDistance)));
  vec3 violetVector=uNeonVioletPosition-vWorldPosition;
  float violetDistance=max(length(violetVector),0.001);
  vec3 violetBounce=evaluatePbrLight(normal,viewDir,violetVector/violetDistance,base.rgb,roughness,metallic,
      uNeonVioletColor*(uNeonVioletIntensity/(1.0+0.18*violetDistance+0.095*violetDistance*violetDistance)));
  vec3 pinkVector=uNeonPinkPosition-vWorldPosition;
  float pinkDistance=max(length(pinkVector),0.001);
  vec3 pinkBounce=evaluatePbrLight(normal,viewDir,pinkVector/pinkDistance,base.rgb,roughness,metallic,
      uNeonPinkColor*(uNeonPinkIntensity/(1.0+0.18*pinkDistance+0.095*pinkDistance*pinkDistance)));
  float ambientOcclusion=0.90+max(normal.y,0.0)*0.10;
  vec3 ambient=base.rgb*(0.15+max(normal.y,0.0)*0.035)*ambientOcclusion;
  vec3 lit=ambient+directional+ceiling+practical+practical2+headboardBounce+blueBounce+violetBounce+pinkBounce;
  outColor=vec4(max(lit,vec3(0.0)),base.a);
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
Mat4 rotateX(float a) { auto m=identity(); const float c=std::cos(a),s=std::sin(a); m[5]=c; m[6]=s; m[9]=-s; m[10]=c; return m; }
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
Mat4 makeView(const XrPosef& pose, float locomotionX, float locomotionZ, float virtualYaw) {
    XrQuaternionf yawQ{0.0f, std::sin(virtualYaw*0.5f), 0.0f, std::cos(virtualYaw*0.5f)};
    XrQuaternionf q=quatMul(yawQ, pose.orientation);
    XrQuaternionf inv{-q.x,-q.y,-q.z,q.w};
    Mat4 r=quatMatrix(inv);
    Mat4 t=translation(-(pose.position.x+locomotionX),-pose.position.y,-(pose.position.z+locomotionZ));
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

std::array<float,3> hsvToRgb(float h,float saturation,float value) {
    h=h-std::floor(h);
    saturation=std::clamp(saturation,0.0f,1.0f);
    value=std::clamp(value,0.0f,1.0f);
    const float sector=h*6.0f;
    const int index=static_cast<int>(std::floor(sector));
    const float fraction=sector-index;
    const float p=value*(1.0f-saturation);
    const float q=value*(1.0f-saturation*fraction);
    const float t=value*(1.0f-saturation*(1.0f-fraction));
    switch(index%6) {
    case 0:return {value,t,p};
    case 1:return {q,value,p};
    case 2:return {p,value,t};
    case 3:return {p,q,value};
    case 4:return {t,p,value};
    default:return {value,p,q};
    }
}
std::array<float,3> rgbToHsv(const std::array<float,3>& color) {
    const float maximum=std::max({color[0],color[1],color[2]});
    const float minimum=std::min({color[0],color[1],color[2]});
    const float delta=maximum-minimum;
    float hue=0.0f;
    if(delta>0.00001f) {
        if(maximum==color[0]) hue=std::fmod((color[1]-color[2])/delta,6.0f);
        else if(maximum==color[1]) hue=(color[2]-color[0])/delta+2.0f;
        else hue=(color[0]-color[1])/delta+4.0f;
        hue/=6.0f;
        if(hue<0.0f) hue+=1.0f;
    }
    const float saturation=maximum<=0.00001f?0.0f:delta/maximum;
    return {hue,saturation,maximum};
}


bool loadRoomFontAtlas() {
    if (gFontAtlasReady && gFontAtlasTexture) return true;
    const char* candidates[] = {
        "/system/fonts/Roboto-Regular.ttf",
        "/system/fonts/Roboto-Medium.ttf",
        "/system/fonts/NotoSans-Regular.ttf"
    };
    std::vector<unsigned char> fontBytes;
    for (const char* path : candidates) {
        std::ifstream input(path, std::ios::binary);
        if (!input.good()) continue;
        input.seekg(0, std::ios::end);
        const std::streamsize size = input.tellg();
        if (size <= 0) continue;
        input.seekg(0, std::ios::beg);
        fontBytes.resize(static_cast<size_t>(size));
        input.read(reinterpret_cast<char*>(fontBytes.data()), size);
        if (input.good()) {
            LOGI("Using smooth UI font: %s", path);
            break;
        }
        fontBytes.clear();
    }
    if (fontBytes.empty()) {
        LOGE("No Android system font could be opened; smooth menu text is unavailable");
        return false;
    }
    std::vector<unsigned char> pixels(static_cast<size_t>(kFontAtlasSize * kFontAtlasSize), 0);
    const int baked = stbtt_BakeFontBitmap(fontBytes.data(), 0, 30.0f,
        pixels.data(), kFontAtlasSize, kFontAtlasSize,
        kFontFirstCharacter, kFontCharacterCount, gFontCharacters.data());
    if (baked <= 0) {
        LOGE("Could not bake the Android UI font atlas");
        return false;
    }
    glGenTextures(1, &gFontAtlasTexture);
    glBindTexture(GL_TEXTURE_2D, gFontAtlasTexture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_ONE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_ONE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_ONE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, kFontAtlasSize, kFontAtlasSize,
                 0, GL_RED, GL_UNSIGNED_BYTE, pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    gFontAtlasReady = gFontAtlasTexture != 0;
    return gFontAtlasReady;
}
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
    if (sceneIndex == 29) {
        // A deep midnight sky leaves a clean horizon line behind the penthouse glazing.
        skyTop = {3.0f, 6.0f, 19.0f};
        skyHorizon = {12.0f, 20.0f, 43.0f};
        ground = {3.0f, 8.0f, 19.0f};
        horizon = 0.80f;
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
    const bool extraNight = sceneIndex == 28 || sceneIndex == 29 || sceneIndex == 30 || sceneIndex == 31 ||
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
            if (sceneIndex == 29 && v > horizon) {
                // Break the city lights into softened, vertically stretched reflections
                // with horizontal ripples so the skyline reads as a waterfront at night.
                const float depth = std::clamp((v - horizon) / (1.0f - horizon), 0.0f, 1.0f);
                const float ripple = 0.5f + 0.5f * std::sin(v * 310.0f + u * 23.0f)
                                            * std::sin(v * 91.0f - u * 13.0f);
                r = 3.0f + depth * 3.0f + ripple * 2.0f;
                g = 8.0f + depth * 6.0f + ripple * 4.0f;
                b = 19.0f + depth * 13.0f + ripple * 8.0f;
                const int building = x / 16;
                const uint32_t seed = hash32(static_cast<uint32_t>(building) * 7919U
                                           + static_cast<uint32_t>(sceneIndex) * 113U + 37U);
                const int localX = x % 16;
                const bool litColumn = localX >= 4 && localX <= 10 && ((seed >> 5) % 7U < 3U);
                const int reflectionRow = static_cast<int>(depth * 92.0f + static_cast<float>((seed >> 12) & 15U));
                if (litColumn && (reflectionRow % 17) < 4) {
                    const float glow = (1.0f - depth * 0.72f) * (0.45f + ripple * 0.55f);
                    if (seed & 1U) { r += 30.0f * glow; g += 105.0f * glow; b += 180.0f * glow; }
                    else { r += 170.0f * glow; g += 35.0f * glow; b += 120.0f * glow; }
                }
                if ((noise % 97U) < 2U && depth < 0.75f) {
                    r += 4.0f; g += 8.0f; b += 13.0f;
                }
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
    if(!ledDefaultsSeeded_) {
        ledZones_[0].primary={.04f,.48f,1.0f}; ledZones_[0].secondary={.58f,.12f,1.0f};
        ledZones_[1].primary={.36f,.10f,1.0f}; ledZones_[1].secondary={.04f,.56f,1.0f};
        ledZones_[2].primary={.04f,.30f,1.0f}; ledZones_[2].secondary={.02f,.72f,1.0f};
        ledZones_[3].primary={.48f,.08f,1.0f}; ledZones_[3].secondary={.10f,.34f,1.0f};
        ledZones_[4].primary={.02f,.72f,.82f}; ledZones_[4].secondary={.18f,.22f,1.0f};
        ledZones_[5].primary={.04f,.34f,1.0f}; ledZones_[5].secondary={.02f,.72f,1.0f};
        ledZones_[6].primary={.52f,.10f,1.0f}; ledZones_[6].secondary={.12f,.22f,1.0f};
        ledDefaultsSeeded_=true;
    }
    GLuint vs=compileShader(GL_VERTEX_SHADER,kVertexShader), fs=compileShader(GL_FRAGMENT_SHADER,kFragmentShader);
    if(!vs||!fs){if(vs)glDeleteShader(vs);if(fs)glDeleteShader(fs);return false;}
    program_=glCreateProgram(); glAttachShader(program_,vs); glAttachShader(program_,fs); glLinkProgram(program_); glDeleteShader(vs); glDeleteShader(fs);
    GLint linked=GL_FALSE; glGetProgramiv(program_,GL_LINK_STATUS,&linked);
    if(linked!=GL_TRUE){char log[1024]{};glGetProgramInfoLog(program_,sizeof(log),nullptr,log);LOGE("room link: %s",log);glDeleteProgram(program_);program_=0;return false;}
    locMvp_=glGetUniformLocation(program_,"uMvp");locColor_=glGetUniformLocation(program_,"uColor");
    locTexture_=glGetUniformLocation(program_,"uTexture");locTextured_=glGetUniformLocation(program_,"uTextured");
    locUvScale_=glGetUniformLocation(program_,"uUvScale");locMaterial_=glGetUniformLocation(program_,"uMaterial");
    locModel_=glGetUniformLocation(program_,"uModel");locRounded_=glGetUniformLocation(program_,"uRounded");
    locBevel_=glGetUniformLocation(program_,"uBevel");
    locCameraPosition_=glGetUniformLocation(program_,"uCameraPosition");
    locRoomAmbientIntensity_=glGetUniformLocation(program_,"uRoomAmbientIntensity");
    locCeilingLightPosition_=glGetUniformLocation(program_,"uCeilingLightPosition");
    locCeilingLightColor_=glGetUniformLocation(program_,"uCeilingLightColor");
    locCeilingLightIntensity_=glGetUniformLocation(program_,"uCeilingLightIntensity");
    locPracticalLightPosition_=glGetUniformLocation(program_,"uPracticalLightPosition");
    locPracticalLightColor_=glGetUniformLocation(program_,"uPracticalLightColor");
    locPracticalLightIntensity_=glGetUniformLocation(program_,"uPracticalLightIntensity");
    locPracticalLight2Position_=glGetUniformLocation(program_,"uPracticalLight2Position");
    locPracticalLight2Color_=glGetUniformLocation(program_,"uPracticalLight2Color");
    locPracticalLight2Intensity_=glGetUniformLocation(program_,"uPracticalLight2Intensity");
    locHeadboardGlowPosition_=glGetUniformLocation(program_,"uHeadboardGlowPosition");
    locHeadboardGlowColor_=glGetUniformLocation(program_,"uHeadboardGlowColor");
    locHeadboardGlowIntensity_=glGetUniformLocation(program_,"uHeadboardGlowIntensity");
    locNeonBluePosition_=glGetUniformLocation(program_,"uNeonBluePosition");
    locNeonBlueColor_=glGetUniformLocation(program_,"uNeonBlueColor");
    locNeonBlueIntensity_=glGetUniformLocation(program_,"uNeonBlueIntensity");
    locNeonVioletPosition_=glGetUniformLocation(program_,"uNeonVioletPosition");
    locNeonVioletColor_=glGetUniformLocation(program_,"uNeonVioletColor");
    locNeonVioletIntensity_=glGetUniformLocation(program_,"uNeonVioletIntensity");
    locNeonPinkPosition_=glGetUniformLocation(program_,"uNeonPinkPosition");
    locNeonPinkColor_=glGetUniformLocation(program_,"uNeonPinkColor");
    locNeonPinkIntensity_=glGetUniformLocation(program_,"uNeonPinkIntensity");
    locMaterialAtlas_=glGetUniformLocation(program_,"uMaterialAtlas");
    locHasMaterialAtlas_=glGetUniformLocation(program_,"uHasMaterialAtlas");
    locNormalAtlas_=glGetUniformLocation(program_,"uNormalAtlas");
    locHasNormalAtlas_=glGetUniformLocation(program_,"uHasNormalAtlas");
    locRoughnessAtlas_=glGetUniformLocation(program_,"uRoughnessAtlas");
    locHasRoughnessAtlas_=glGetUniformLocation(program_,"uHasRoughnessAtlas");
    GLuint vvs=compileShader(GL_VERTEX_SHADER,kVignetteVertex), vfs=compileShader(GL_FRAGMENT_SHADER,kVignetteFragment);
    if(vvs&&vfs){vignetteProgram_=glCreateProgram();glAttachShader(vignetteProgram_,vvs);glAttachShader(vignetteProgram_,vfs);glLinkProgram(vignetteProgram_);GLint vok=GL_FALSE;glGetProgramiv(vignetteProgram_,GL_LINK_STATUS,&vok);if(vok!=GL_TRUE){glDeleteProgram(vignetteProgram_);vignetteProgram_=0;}}
    if(vvs)glDeleteShader(vvs);if(vfs)glDeleteShader(vfs);
    createGeometry();
    if (!loadRoomFontAtlas()) LOGE("QuestBridge menu will not render text without a system font");
    const bool bedLoaded=bedModel_.load(assets,"models/arc_upholstered_bed.glb");
    const bool deskLoaded=deskModel_.load(assets,"models/arc_gaming_desk.glb");
    const bool monitorLoaded=monitorModel_.load(assets,"models/arc27_curved_monitor.glb");
    LOGI("custom furniture GLB status: bed=%d desk=%d monitor=%d",
         bedLoaded?1:0,deskLoaded?1:0,monitorLoaded?1:0);
    environmentTextures_[0]=loadRgbAsset(assets,"environments/blossom.rgb");
    environmentTextures_[1]=loadRgbAsset(assets,"environments/night_city.rgb");
    environmentTextures_[2]=loadRgbAsset(assets,"environments/golden_city.rgb");
    materialAtlasTexture_=loadRgbAsset(assets,"materials/room_surface_atlas.rgb");
    normalAtlasTexture_=loadRgbAsset(assets,"materials/room_normal_atlas.rgb");
    roughnessAtlasTexture_=loadRgbAsset(assets,"materials/room_roughness_atlas.rgb");
    if(roughnessAtlasTexture_) {
        glBindTexture(GL_TEXTURE_2D,roughnessAtlasTexture_);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D,0);
        LOGI("loaded material roughness atlas");
    }
    if(normalAtlasTexture_) {
        glBindTexture(GL_TEXTURE_2D,normalAtlasTexture_);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D,0);
        LOGI("loaded tangent-space normal atlas");
    }
    if(materialAtlasTexture_) {
        glBindTexture(GL_TEXTURE_2D,materialAtlasTexture_);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D,0);
        LOGI("loaded room surface material atlas");
    } else {
        LOGW("room surface material atlas missing; procedural material fallback active");
    }
    // A harmless fallback texture gives an intentional dark idle screen before a PC stream starts.
    const uint8_t idlePixels[4*4*3] = {
      18,24,39, 20,28,45, 20,28,45, 18,24,39, 20,28,45, 35,42,63, 35,42,63, 20,28,45,
      20,28,45, 35,42,63, 46,50,74, 20,28,45, 20,28,45, 35,42,63, 35,42,63, 20,28,45};
    glGenTextures(1,&videoFallbackTexture_);glBindTexture(GL_TEXTURE_2D,videoFallbackTexture_);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,4,4,0,GL_RGB,GL_UNSIGNED_BYTE,idlePixels);glBindTexture(GL_TEXTURE_2D,0);
    // Soft radial contact-shadow texture, generated locally to avoid runtime downloads.
    constexpr int shadowSize=96;
    std::vector<uint8_t> shadowPixels(static_cast<size_t>(shadowSize*shadowSize*4),0);
    for(int py=0;py<shadowSize;++py) for(int px=0;px<shadowSize;++px) {
        const float nx=(2.0f*(px+0.5f)/shadowSize)-1.0f;
        const float ny=(2.0f*(py+0.5f)/shadowSize)-1.0f;
        const float radius=std::sqrt(nx*nx+ny*ny);
        const float edge=std::clamp((1.0f-radius)/0.22f,0.0f,1.0f);
        const float alpha=0.34f*std::exp(-radius*radius*4.8f)*edge;
        const size_t offset=static_cast<size_t>((py*shadowSize+px)*4);
        shadowPixels[offset+0]=10;shadowPixels[offset+1]=12;shadowPixels[offset+2]=17;
        shadowPixels[offset+3]=static_cast<uint8_t>(std::clamp(alpha*255.0f,0.0f,255.0f));
    }
    glGenTextures(1,&groundShadowTexture_);
    glBindTexture(GL_TEXTURE_2D,groundShadowTexture_);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,shadowSize,shadowSize,0,GL_RGBA,GL_UNSIGNED_BYTE,shadowPixels.data());
    glBindTexture(GL_TEXTURE_2D,0);
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
    // Subdivided cube surface lets the vertex shader produce true multi-segment bevels.
    // This mesh is shared by the desk, PC chassis, drawers and other hard-surface furniture.
    std::vector<Vertex> roundedVertices;
    constexpr int bevelGrid = 24;
    auto roundedPoint = [](int axis,int sign,float u,float v) {
        Vertex out{};
        if(axis==0){out.x=sign*0.5f;out.y=u-0.5f;out.z=v-0.5f;}
        else if(axis==1){out.x=u-0.5f;out.y=sign*0.5f;out.z=v-0.5f;}
        else {out.x=u-0.5f;out.y=v-0.5f;out.z=sign*0.5f;}
        out.u=u;out.v=v;
        return out;
    };
    roundedVertices.reserve(6u*2u*bevelGrid*bevelGrid*6u);
    for(int axis=0;axis<3;++axis) for(int sign : {-1,1}) {
        for(int row=0;row<bevelGrid;++row) for(int col=0;col<bevelGrid;++col) {
            const float u0=float(col)/bevelGrid,u1=float(col+1)/bevelGrid;
            const float v0=float(row)/bevelGrid,v1=float(row+1)/bevelGrid;
            const Vertex a=roundedPoint(axis,sign,u0,v0);
            const Vertex b=roundedPoint(axis,sign,u1,v0);
            const Vertex c=roundedPoint(axis,sign,u1,v1);
            const Vertex d=roundedPoint(axis,sign,u0,v1);
            roundedVertices.insert(roundedVertices.end(),{a,b,c,a,c,d});
        }
    }
    roundedVertexCount_=static_cast<GLsizei>(roundedVertices.size());
    glGenVertexArrays(1,&roundedVao_);
    glGenBuffers(1,&roundedVbo_);
    glBindVertexArray(roundedVao_);
    glBindBuffer(GL_ARRAY_BUFFER,roundedVbo_);
    glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(roundedVertices.size()*sizeof(Vertex)),roundedVertices.data(),GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(3*sizeof(float)));
    glBindVertexArray(0);

    const float plane[] = {-0.5f,-0.5f,0,0,1, 0.5f,-0.5f,0,1,1, 0.5f,0.5f,0,1,0, -0.5f,-0.5f,0,0,1, 0.5f,0.5f,0,1,0, -0.5f,0.5f,0,0,0};
    glGenVertexArrays(1,&planeTexture_); // plane VAO handles geometry separately
    glGenBuffers(1,&cubeTexture_); // IDs are reused as plane VAO/VBO handles; no texture objects here
    glBindVertexArray(planeTexture_);glBindBuffer(GL_ARRAY_BUFFER,cubeTexture_);glBufferData(GL_ARRAY_BUFFER,sizeof(plane),plane,GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,5*sizeof(float),reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,5*sizeof(float),reinterpret_cast<void*>(3*sizeof(float)));
    glBindVertexArray(0);

    // One shared tapered cylinder mesh for ceramic planters, cups, gas lifts and table legs.
    std::vector<Vertex> cylinderVertices;
    constexpr int cylinderSegments=40;
    cylinderVertices.reserve(static_cast<size_t>(cylinderSegments)*12u);
    auto cylinderPoint=[](float angle,float y,float radius,float u) {
        return Vertex{std::cos(angle)*radius,y,std::sin(angle)*radius,u,y+0.5f};
    };
    for(int segment=0;segment<cylinderSegments;++segment) {
        const float u0=float(segment)/cylinderSegments,u1=float(segment+1)/cylinderSegments;
        const float a0=u0*2.0f*kPi,a1=u1*2.0f*kPi;
        const Vertex b0=cylinderPoint(a0,-.5f,.42f,u0),b1=cylinderPoint(a1,-.5f,.42f,u1);
        const Vertex t0=cylinderPoint(a0,.5f,.50f,u0),t1=cylinderPoint(a1,.5f,.50f,u1);
        cylinderVertices.insert(cylinderVertices.end(),{b0,b1,t1,b0,t1,t0});
        const Vertex bottomCenter{0,-.5f,0,.5f,.5f};
        const Vertex topCenter{0,.5f,0,.5f,.5f};
        cylinderVertices.insert(cylinderVertices.end(),{bottomCenter,b1,b0,topCenter,t0,t1});
    }
    cylinderVertexCount_=static_cast<GLsizei>(cylinderVertices.size());
    glGenVertexArrays(1,&cylinderVao_);
    glGenBuffers(1,&cylinderVbo_);
    glBindVertexArray(cylinderVao_);
    glBindBuffer(GL_ARRAY_BUFFER,cylinderVbo_);
    glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(cylinderVertices.size()*sizeof(Vertex)),cylinderVertices.data(),GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(3*sizeof(float)));
    glBindVertexArray(0);

    // Parametric upholstery mesh: domed faces, softly bulged side walls and closed seams.
    // Unlike flattened spheres, this gives seat cushions and back cushions a tailored,
    // rectangular silhouette with a believable padded crown.
    std::vector<Vertex> pillowVertices;
    constexpr int pillowGrid=20;
    auto topY=[](float u,float v) {
        const float px=std::max(0.0f,1.0f-4.0f*u*u);
        const float pz=std::max(0.0f,1.0f-4.0f*v*v);
        return 0.255f+0.205f*std::pow(px,0.72f)*std::pow(pz,0.72f);
    };
    auto addTri=[&](const Vertex& a,const Vertex& b,const Vertex& c) {
        pillowVertices.push_back(a);pillowVertices.push_back(b);pillowVertices.push_back(c);
    };
    auto topPoint=[&](float u,float v) {
        return Vertex{u,topY(u,v),v,u+0.5f,v+0.5f};
    };
    auto bottomPoint=[](float u,float v) {
        return Vertex{u,-0.255f,v,u+0.5f,v+0.5f};
    };
    for(int row=0;row<pillowGrid;++row) for(int col=0;col<pillowGrid;++col) {
        const float x0=-0.5f+float(col)/pillowGrid;
        const float x1=-0.5f+float(col+1)/pillowGrid;
        const float z0=-0.5f+float(row)/pillowGrid;
        const float z1=-0.5f+float(row+1)/pillowGrid;
        const Vertex a=topPoint(x0,z0),b=topPoint(x1,z0),c=topPoint(x1,z1),d=topPoint(x0,z1);
        addTri(a,d,b);addTri(b,d,c); // top surface faces +Y
        const Vertex ba=bottomPoint(x0,z0),bb=bottomPoint(x1,z0);
        const Vertex bc=bottomPoint(x1,z1),bd=bottomPoint(x0,z1);
        addTri(ba,bb,bd);addTri(bb,bc,bd); // closed underside
    }
    // Front and rear gussets have a slight outward belly between their welt seams.
    for(int row=0;row<pillowGrid;++row) {
        const float x0=-0.5f+float(row)/pillowGrid;
        const float x1=-0.5f+float(row+1)/pillowGrid;
        for(int step=0;step<8;++step) {
            const float t0=float(step)/8.0f,t1=float(step+1)/8.0f;
            const float y0=-0.255f+0.510f*t0,y1=-0.255f+0.510f*t1;
            const float bulge0=0.035f*std::sin(kPi*t0),bulge1=0.035f*std::sin(kPi*t1);
            for(int side : {-1,1}) {
                const Vertex a{x0,y0,side*(0.5f+bulge0),x0+0.5f,t0};
                const Vertex b{x1,y0,side*(0.5f+bulge0),x1+0.5f,t0};
                const Vertex c{x1,y1,side*(0.5f+bulge1),x1+0.5f,t1};
                const Vertex d{x0,y1,side*(0.5f+bulge1),x0+0.5f,t1};
                if(side>0){addTri(a,b,c);addTri(a,c,d);}
                else {addTri(a,c,b);addTri(a,d,c);}
            }
        }
    }
    // Left and right gussets use opposite winding, keeping the pillow watertight.
    for(int row=0;row<pillowGrid;++row) {
        const float z0=-0.5f+float(row)/pillowGrid;
        const float z1=-0.5f+float(row+1)/pillowGrid;
        for(int step=0;step<8;++step) {
            const float t0=float(step)/8.0f,t1=float(step+1)/8.0f;
            const float y0=-0.255f+0.510f*t0,y1=-0.255f+0.510f*t1;
            const float bulge0=0.035f*std::sin(kPi*t0),bulge1=0.035f*std::sin(kPi*t1);
            for(int side : {-1,1}) {
                const float x0=side*(0.5f+bulge0),x1=side*(0.5f+bulge1);
                const Vertex a{x0,y0,z0,float(row)/pillowGrid,t0};
                const Vertex b{x0,y0,z1,float(row+1)/pillowGrid,t0};
                const Vertex c{x1,y1,z1,float(row+1)/pillowGrid,t1};
                const Vertex d{x1,y1,z0,float(row)/pillowGrid,t1};
                if(side<0){addTri(a,b,c);addTri(a,c,d);}
                else {addTri(a,c,b);addTri(a,d,c);}
            }
        }
    }
    pillowVertexCount_=static_cast<GLsizei>(pillowVertices.size());
    glGenVertexArrays(1,&pillowVao_);
    glGenBuffers(1,&pillowVbo_);
    glBindVertexArray(pillowVao_);
    glBindBuffer(GL_ARRAY_BUFFER,pillowVbo_);
    glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(pillowVertices.size()*sizeof(Vertex)),pillowVertices.data(),GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(3*sizeof(float)));
    glBindVertexArray(0);

    // Pleated cloth panel mesh: repeated vertical folds are part of the surface,
    // not stacked primitive ellipsoids. The wave amplitude eases near the hems.
    std::vector<Vertex> curtainVertices;
    constexpr int curtainColumns=48,curtainRows=18;
    auto curtainPoint=[](float u,float v) {
        const float hemFade=0.78f+0.22f*std::sin(kPi*v);
        const float pleat=std::cos(u*2.0f*kPi*8.0f);
        const float secondary=std::sin(u*2.0f*kPi*3.0f+0.45f);
        const float z=0.034f*pleat*hemFade+0.008f*secondary*std::sin(kPi*v);
        return Vertex{u-0.5f,v-0.5f,z,u*1.8f,v};
    };
    curtainVertices.reserve(static_cast<size_t>(curtainColumns)*curtainRows*6u);
    for(int row=0;row<curtainRows;++row) for(int col=0;col<curtainColumns;++col) {
        const float u0=float(col)/curtainColumns,u1=float(col+1)/curtainColumns;
        const float v0=float(row)/curtainRows,v1=float(row+1)/curtainRows;
        const Vertex a=curtainPoint(u0,v0),b=curtainPoint(u1,v0);
        const Vertex c=curtainPoint(u1,v1),d=curtainPoint(u0,v1);
        curtainVertices.insert(curtainVertices.end(),{a,b,c,a,c,d});
    }
    curtainVertexCount_=static_cast<GLsizei>(curtainVertices.size());
    glGenVertexArrays(1,&curtainVao_);
    glGenBuffers(1,&curtainVbo_);
    glBindVertexArray(curtainVao_);
    glBindBuffer(GL_ARRAY_BUFFER,curtainVbo_);
    glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(curtainVertices.size()*sizeof(Vertex)),curtainVertices.data(),GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(3*sizeof(float)));
    glBindVertexArray(0);

    // Smooth UV sphere mesh used for rounded room details and equipment hardware.
    // It is generated once and shared by every room object to keep draw overhead low.
    std::vector<float> sphere;
    constexpr int rings = 24, segments = 32;
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

std::array<float,3> RoomRenderer::ledZoneColor(int zone,float position) const {
    if(zone<0||zone>=static_cast<int>(ledZones_.size())) return {0.0f,0.0f,0.0f};
    const auto& led=ledZones_[static_cast<size_t>(zone)];
    if(!led.enabled) return {0.0f,0.0f,0.0f};
    const float t=led.gradient?std::clamp(position,0.0f,1.0f):0.0f;
    const float brightness=std::clamp(led.brightness,0.0f,1.0f);
    return {
        std::clamp((led.primary[0]*(1.0f-t)+led.secondary[0]*t)*brightness,0.0f,1.0f),
        std::clamp((led.primary[1]*(1.0f-t)+led.secondary[1]*t)*brightness,0.0f,1.0f),
        std::clamp((led.primary[2]*(1.0f-t)+led.secondary[2]*t)*brightness,0.0f,1.0f)
    };
}
std::array<float,3> RoomRenderer::ledZoneAverageColor(int zone) const {
    return ledZoneColor(zone,0.5f);
}
void RoomRenderer::applyLedPreset(int preset) {
    static const std::array<std::array<float,3>,6> presets{{
        {{1.0f,.10f,.045f}}, {{.04f,.86f,.16f}}, {{.08f,.34f,1.0f}},
        {{.78f,.84f,1.0f}}, {{.55f,.08f,1.0f}}, {{.02f,.78f,.82f}}
    }};
    static const std::array<std::array<float,3>,6> companions{{
        {{1.0f,.32f,.04f}}, {{.24f,1.0f,.12f}}, {{.50f,.12f,1.0f}},
        {{.48f,.62f,.92f}}, {{1.0f,.10f,.62f}}, {{.04f,.46f,.72f}}
    }};
    const int index=std::clamp(preset,0,5);
    for(size_t zone=0;zone<ledZones_.size();++zone) {
        ledZones_[zone].primary=presets[static_cast<size_t>((index+static_cast<int>(zone)%3)%6)];
        ledZones_[zone].secondary=companions[static_cast<size_t>((index+static_cast<int>(zone)%3)%6)];
        ledZones_[zone].gradient=false;
        ledZones_[zone].enabled=true;
    }
    ledPreset_=index;
}
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
    previousMenuHorizontal_ = false;
    previousMenuAdjust_ = false;
    environmentDropdownOpen_ = false;
    lightingColorPickerOpen_ = false;
    if (menuVisible_) menuSelection_ = 0;
}

void RoomRenderer::setConnectionState(bool connected, const std::string& hostIp, bool streaming) {
    pcConnected_ = connected;
    pcStreaming_ = streaming;
    pcHostIp_ = hostIp.empty() ? "Not configured" : hostIp;
}

bool RoomRenderer::consumeReconnectRequest() {
    const bool requested = reconnectRequested_;
    reconnectRequested_ = false;
    return requested;
}

void RoomRenderer::handleMenuInput(float horizontal,float vertical,float adjust,bool selectPressed) {
    if(!menuVisible_) return;
    const bool navigatingVertical=std::fabs(vertical)>0.58f;
    const bool navigatingHorizontal=std::fabs(horizontal)>0.68f;
    const bool adjusting=std::fabs(adjust)>0.70f;

    // In the HSV picker, the left stick controls hue and saturation; the
    // right stick controls value. A returns to the Lighting page.
    if(lightingColorPickerOpen_&&menuPage_==3) {
        if(selectPressed) {
            lightingColorPickerOpen_=false;
        } else {
            lightingPickerHue_=std::fmod(lightingPickerHue_+horizontal*0.0014f+1.0f,1.0f);
            lightingPickerSaturation_=std::clamp(lightingPickerSaturation_+vertical*0.0015f,0.0f,1.0f);
            lightingPickerValue_=std::clamp(lightingPickerValue_+adjust*0.0015f,0.04f,1.0f);
            const auto picked=hsvToRgb(lightingPickerHue_,lightingPickerSaturation_,lightingPickerValue_);
            auto& zone=ledZones_[static_cast<size_t>(lightingZoneIndex_)];
            if(lightingColorTarget_==0) zone.primary=picked;
            else zone.secondary=picked;
            ledPreset_=-1;
        }
        previousMenuNavigation_=false;
        previousMenuHorizontal_=std::fabs(horizontal)>0.08f;
        previousMenuAdjust_=std::fabs(adjust)>0.08f;
        return;
    }

    if(navigatingHorizontal&&!previousMenuHorizontal_&&!environmentDropdownOpen_) {
        menuPage_=(menuPage_+(horizontal>0.0f?1:4))%5;
        menuSelection_=0;
    } else if(navigatingVertical&&!previousMenuNavigation_) {
        if(environmentDropdownOpen_) {
            environmentDropdownIndex_=(environmentDropdownIndex_+
                (vertical>0.0f?static_cast<int>(kEnvironmentNames.size())-1:1))%
                static_cast<int>(kEnvironmentNames.size());
        } else {
            const int rowCounts[5]={1,5,3,6,4};
            const int count=rowCounts[menuPage_];
            menuSelection_=(menuSelection_+(vertical>0.0f?count-1:1))%count;
        }
    }

    auto adjustSelected=[&](int direction) {
        if(menuPage_==1) {
            if(menuSelection_==0) {
                environmentDropdownIndex_=(environmentDropdownIndex_+
                    (direction>0?1:static_cast<int>(kEnvironmentNames.size())-1))%
                    static_cast<int>(kEnvironmentNames.size());
                settings_.environment=kEnvironmentNames[environmentDropdownIndex_];
                chooseEnvironment();
            } else if(menuSelection_==1) {
                const char* sizes[]={"small","medium","large"};
                const int index=settings_.roomSize=="small"?0:(settings_.roomSize=="large"?2:1);
                settings_.roomSize=sizes[(index+(direction>0?1:2))%3];
            } else if(menuSelection_==3) {
                settings_.movementSpeed=std::clamp(settings_.movementSpeed+direction*0.10f,0.30f,3.00f);
            }
        } else if(menuPage_==2) {
            if(menuSelection_==0) {
                const char* modes[]={"monitor","tv","both"};
                const int index=settings_.screenDestination=="tv"?1:
                    (settings_.screenDestination=="both"?2:0);
                settings_.screenDestination=modes[(index+(direction>0?1:2))%3];
            } else if(menuSelection_==2) {
                settings_.monitorCount=std::clamp(settings_.monitorCount+direction,1,3);
            }
        } else if(menuPage_==3) {
            if(menuSelection_==0) {
                lightingZoneIndex_=(lightingZoneIndex_+(direction>0?1:6))%7;
            } else if(menuSelection_==1) {
                auto& zone=ledZones_[static_cast<size_t>(lightingZoneIndex_)];
                zone.gradient=!zone.gradient;
            } else if(menuSelection_==4) {
                auto& zone=ledZones_[static_cast<size_t>(lightingZoneIndex_)];
                zone.brightness=std::clamp(zone.brightness+direction*0.05f,0.0f,1.0f);
            }
            ledPreset_=-1;
        } else if(menuPage_==4) {
            if(menuSelection_==1) {
                settings_.turnSpeed=std::clamp(settings_.turnSpeed+direction*0.10f,0.2f,3.5f);
            } else if(menuSelection_==2) {
                settings_.snapTurnDegrees=std::clamp(settings_.snapTurnDegrees+direction*15.0f,15.0f,60.0f);
            }
        }
    };

    if(adjusting&&!previousMenuAdjust_) {
        if(environmentDropdownOpen_) {
            environmentDropdownIndex_=(environmentDropdownIndex_+
                (adjust>0.0f?1:static_cast<int>(kEnvironmentNames.size())-1))%
                static_cast<int>(kEnvironmentNames.size());
        } else {
            adjustSelected(adjust>0.0f?1:-1);
        }
    }

    if(selectPressed) {
        if(environmentDropdownOpen_) {
            settings_.environment=kEnvironmentNames[environmentDropdownIndex_];
            chooseEnvironment();
            environmentDropdownOpen_=false;
        } else if(menuPage_==0) {
            reconnectRequested_=true;
        } else if(menuPage_==1) {
            switch(menuSelection_) {
            case 0:
                environmentDropdownOpen_=true;
                environmentDropdownIndex_=activeEnvironmentIndex_;
                break;
            case 1: adjustSelected(1); break;
            case 2: settings_.roomMovementEnabled=!settings_.roomMovementEnabled; break;
            case 4:
                moveX_=0.0f;
                moveZ_=0.0f;
                virtualYaw_=0.0f;
                snapCooldown_=0.0f;
                previousTurnRight_=previousTurnLeft_=false;
                break;
            default: break;
            }
        } else if(menuPage_==2) {
            if(menuSelection_==0) adjustSelected(1);
            else if(menuSelection_==1) settings_.showTv=!settings_.showTv;
            else if(menuSelection_==2) adjustSelected(1);
        } else if(menuPage_==3) {
            auto& zone=ledZones_[static_cast<size_t>(lightingZoneIndex_)];
            if(menuSelection_==0) {
                lightingZoneIndex_=(lightingZoneIndex_+1)%7;
            } else if(menuSelection_==1) {
                zone.gradient=!zone.gradient;
                ledPreset_=-1;
            } else if(menuSelection_==2||menuSelection_==3) {
                lightingColorTarget_=menuSelection_==2?0:1;
                const auto& color=lightingColorTarget_==0?zone.primary:zone.secondary;
                const auto hsv=rgbToHsv(color);
                lightingPickerHue_=hsv[0];
                lightingPickerSaturation_=hsv[1];
                lightingPickerValue_=std::max(0.04f,hsv[2]);
                lightingColorPickerOpen_=true;
            } else if(menuSelection_==4) {
                zone.brightness=std::clamp(zone.brightness+0.05f,0.0f,1.0f);
                ledPreset_=-1;
            } else if(menuSelection_==5) {
                zone.enabled=!zone.enabled;
                ledPreset_=-1;
            }
        } else if(menuPage_==4) {
            if(menuSelection_==0) settings_.smoothTurn=!settings_.smoothTurn;
            else if(menuSelection_==3) settings_.comfortVignette=!settings_.comfortVignette;
            else if(menuSelection_==1||menuSelection_==2) adjustSelected(1);
        }
    }
    previousMenuNavigation_=navigatingVertical;
    previousMenuHorizontal_=navigatingHorizontal;
    previousMenuAdjust_=adjusting;
}

void RoomRenderer::drawBox(const float* pv,float x,float y,float z,float sx,float sy,float sz,const std::array<float,4>& color,float yaw,int material) {
    Mat4 projectionView{}; std::copy(pv, pv + projectionView.size(), projectionView.begin());
    Mat4 model=mul(translation(x,y,z),mul(rotateY(yaw),scaling(sx,sy,sz))); Mat4 mvp=mul(projectionView,model);
    glUniformMatrix4fv(locMvp_,1,GL_FALSE,mvp.data());glUniformMatrix4fv(locModel_,1,GL_FALSE,model.data());
    glUniform4fv(locColor_,1,color.data());glUniform1i(locTextured_,0);glUniform1i(locMaterial_,material);
    glUniform1i(locRounded_,0);
    const float scaleX=material==6?std::max(1.0f,sx*.60f):std::max(1.0f,std::max(sx,sz)*2.6f);
    const float scaleY=material==6?std::max(1.0f,sz*.92f):std::max(1.0f,std::max(sy,sz)*2.6f);
    glUniform2f(locUvScale_,scaleX,scaleY);
    glBindVertexArray(vao_);glDrawArrays(GL_TRIANGLES,0,36);glBindVertexArray(0);
}
void RoomRenderer::drawPillow(const float* pv,float x,float y,float z,float sx,float sy,float sz,
                               const std::array<float,4>& color,float yaw,float pitch,int material) {
    Mat4 projectionView{};std::copy(pv,pv+projectionView.size(),projectionView.begin());
    const Mat4 orientation=mul(rotateY(yaw),rotateX(pitch));
    const Mat4 model=mul(translation(x,y,z),mul(orientation,scaling(sx,sy,sz)));
    const Mat4 mvp=mul(projectionView,model);
    glUniformMatrix4fv(locMvp_,1,GL_FALSE,mvp.data());
    glUniformMatrix4fv(locModel_,1,GL_FALSE,model.data());
    glUniform4fv(locColor_,1,color.data());
    glUniform1i(locTextured_,0);glUniform1i(locMaterial_,material);glUniform1i(locRounded_,0);
    glUniform2f(locUvScale_,std::max(1.0f,sx*2.8f),std::max(1.0f,sz*2.8f));
    glBindVertexArray(pillowVao_);
    glDrawArrays(GL_TRIANGLES,0,pillowVertexCount_);
    glBindVertexArray(0);
}

void RoomRenderer::drawCurtainPanel(const float* pv,float x,float y,float z,float width,float height,
                                      const std::array<float,4>& color,float yaw) {
    Mat4 projectionView{};std::copy(pv,pv+projectionView.size(),projectionView.begin());
    const Mat4 model=mul(translation(x,y,z),mul(rotateY(yaw),scaling(width,height,1.0f)));
    const Mat4 mvp=mul(projectionView,model);
    glUniformMatrix4fv(locMvp_,1,GL_FALSE,mvp.data());
    glUniformMatrix4fv(locModel_,1,GL_FALSE,model.data());
    glUniform4fv(locColor_,1,color.data());
    glUniform1i(locTextured_,0);glUniform1i(locMaterial_,3);glUniform1i(locRounded_,0);
    glUniform2f(locUvScale_,std::max(1.0f,width*3.2f),std::max(1.0f,height*1.4f));
    glBindVertexArray(curtainVao_);
    glDrawArrays(GL_TRIANGLES,0,curtainVertexCount_);
    glBindVertexArray(0);
}

void RoomRenderer::drawCylinder(const float* pv,float x,float y,float z,float diameter,float height,
                                const std::array<float,4>& color,int material,float pitch) {
    Mat4 projectionView{};std::copy(pv,pv+projectionView.size(),projectionView.begin());
    const Mat4 model=mul(translation(x,y,z),mul(rotateX(pitch),scaling(diameter,height,diameter)));
    const Mat4 mvp=mul(projectionView,model);
    glUniformMatrix4fv(locMvp_,1,GL_FALSE,mvp.data());
    glUniformMatrix4fv(locModel_,1,GL_FALSE,model.data());
    glUniform4fv(locColor_,1,color.data());
    glUniform1i(locTextured_,0);glUniform1i(locMaterial_,material);glUniform1i(locRounded_,0);
    glUniform2f(locUvScale_,1.0f,1.0f);
    glBindVertexArray(cylinderVao_);
    glDrawArrays(GL_TRIANGLES,0,cylinderVertexCount_);
    glBindVertexArray(0);
}

void RoomRenderer::drawRoundedBox(const float* pv,float x,float y,float z,float sx,float sy,float sz,
                                      const std::array<float,4>& color,float yaw,int material) {
    Mat4 projectionView{}; std::copy(pv,pv+projectionView.size(),projectionView.begin());
    const Mat4 model=mul(translation(x,y,z),mul(rotateY(yaw),scaling(sx,sy,sz)));
    const Mat4 mvp=mul(projectionView,model);
    glUniformMatrix4fv(locMvp_,1,GL_FALSE,mvp.data());
    glUniformMatrix4fv(locModel_,1,GL_FALSE,model.data());
    glUniform4fv(locColor_,1,color.data());
    glUniform1i(locTextured_,0);glUniform1i(locMaterial_,material);glUniform1i(locRounded_,1);
    const float bevelWorld=std::min(0.025f,std::min({sx,sy,sz})*0.18f);
    glUniform3f(locBevel_,std::clamp(bevelWorld/std::max(sx,0.001f),0.005f,0.24f),
                           std::clamp(bevelWorld/std::max(sy,0.001f),0.005f,0.24f),
                           std::clamp(bevelWorld/std::max(sz,0.001f),0.005f,0.24f));
    const float scaleX=material==6?std::max(1.0f,sx*.60f):std::max(1.0f,std::max(sx,sz)*2.6f);
    const float scaleY=material==6?std::max(1.0f,sz*.92f):std::max(1.0f,std::max(sy,sz)*2.6f);
    glUniform2f(locUvScale_,scaleX,scaleY);
    glBindVertexArray(roundedVao_);
    glDrawArrays(GL_TRIANGLES,0,roundedVertexCount_);
    glBindVertexArray(0);
}
void RoomRenderer::drawText(const float* pv, const char* text, float x, float y, float z,
                            float pixelSize, const std::array<float,4>& color,
                            float curveCenterX, float curveWidth, float curveDepth, float curveCenterY) {
    if (!text || !gFontAtlasReady || !gFontAtlasTexture) return;
    std::vector<Vertex> vertices;
    vertices.reserve(std::strlen(text) * 6u);
    const float scale = std::max(pixelSize, 0.0001f) * 0.25f;
    float penX = 0.0f, penY = 0.0f;
    const float pitchCos = std::cos(kMonitorScreenPitch);
    const float pitchSin = std::sin(kMonitorScreenPitch);
    auto add = [&](float px, float py, float u, float v) {
        float projectedY = py;
        float projectedZ = z;
        if (curveWidth > 0.0001f && curveDepth > 0.0f) {
            const float normalized = std::clamp((px - curveCenterX) / (curveWidth * 0.5f), -1.0f, 1.0f);
            // The GLB's screen recesses at the center and bends toward the viewer at its edges.
            const float localZ = curveDepth * normalized * normalized;
            const float localY = py - curveCenterY;
            projectedY = curveCenterY + pitchCos * localY + pitchSin * localZ;
            projectedZ = z - pitchSin * localY + pitchCos * localZ;
        }
        vertices.push_back({px, projectedY, projectedZ, u, v});
    };
    for (int i = 0; text[i] != '\0'; ++i) {
        unsigned char character = static_cast<unsigned char>(text[i]);
        if (character == '\n') { penX = 0.0f; penY += 34.0f; continue; }
        if (character < kFontFirstCharacter || character >= kFontFirstCharacter + kFontCharacterCount)
            character = '?';
        stbtt_aligned_quad q{};
        stbtt_GetBakedQuad(gFontCharacters.data(), kFontAtlasSize, kFontAtlasSize,
                           character - kFontFirstCharacter, &penX, &penY, &q, 1);
        const float x0=x+q.x0*scale, x1=x+q.x1*scale;
        const float y0=y-q.y0*scale, y1=y-q.y1*scale;
        add(x0,y0,q.s0,q.t0); add(x1,y0,q.s1,q.t0); add(x1,y1,q.s1,q.t1);
        add(x0,y0,q.s0,q.t0); add(x1,y1,q.s1,q.t1); add(x0,y1,q.s0,q.t1);
    }
    if (vertices.empty()) return;
    if (!textVao_) {
        glGenVertexArrays(1,&textVao_); glGenBuffers(1,&textVbo_);
        glBindVertexArray(textVao_); glBindBuffer(GL_ARRAY_BUFFER,textVbo_);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),nullptr);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(3*sizeof(float)));
        glBindVertexArray(0);
    }
    glUniformMatrix4fv(locMvp_,1,GL_FALSE,pv);
    const Mat4 textModel=identity();
    glUniformMatrix4fv(locModel_,1,GL_FALSE,textModel.data());
    glUniform4fv(locColor_,1,color.data());
    glUniform1i(locTextured_,1); glUniform1i(locMaterial_,-1); glUniform1i(locRounded_,0);
    glUniform2f(locUvScale_,1.0f,1.0f);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,gFontAtlasTexture);
    glUniform1i(locTexture_,0);
    glBindVertexArray(textVao_); glBindBuffer(GL_ARRAY_BUFFER,textVbo_);
    glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Vertex),vertices.data(),GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES,0,static_cast<GLsizei>(vertices.size()));
    glBindVertexArray(0); glBindBuffer(GL_ARRAY_BUFFER,0); glBindTexture(GL_TEXTURE_2D,0);
}

void RoomRenderer::drawSphere(const float* pv,float x,float y,float z,float sx,float sy,float sz,const std::array<float,4>& color,int material) {
    Mat4 projectionView{}; std::copy(pv,pv+projectionView.size(),projectionView.begin());
    const Mat4 model=mul(translation(x,y,z),scaling(sx,sy,sz));
    const Mat4 mvp=mul(projectionView,model);
    glUniformMatrix4fv(locMvp_,1,GL_FALSE,mvp.data());glUniformMatrix4fv(locModel_,1,GL_FALSE,model.data());
    glUniform4fv(locColor_,1,color.data());
    glUniform1i(locTextured_,0);glUniform1i(locMaterial_,material);glUniform2f(locUvScale_,1.0f,1.0f);
    glUniform1i(locRounded_,0);
    glBindVertexArray(sphereVao_);
    glDrawArrays(GL_TRIANGLES,0,sphereVertexCount_);
    glBindVertexArray(0);
}
void RoomRenderer::drawPlane(const float* pv,float x,float y,float z,float sx,float sy,GLuint texture,const std::array<float,4>& color,float yaw) {
    Mat4 projectionView{}; std::copy(pv, pv + projectionView.size(), projectionView.begin());
    Mat4 model=mul(translation(x,y,z),mul(rotateY(yaw),scaling(sx,sy,1)));Mat4 mvp=mul(projectionView,model);
    glUniformMatrix4fv(locMvp_,1,GL_FALSE,mvp.data());glUniformMatrix4fv(locModel_,1,GL_FALSE,model.data());
    glUniform4fv(locColor_,1,color.data());glUniform1i(locTextured_,texture?1:0);glUniform1i(locMaterial_,-1);glUniform1i(locRounded_,0);
    glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,texture?texture:videoFallbackTexture_);glUniform1i(locTexture_,0);
    glBindVertexArray(planeTexture_);glDrawArrays(GL_TRIANGLES,0,6);glBindVertexArray(0);glBindTexture(GL_TEXTURE_2D,0);
}

void RoomRenderer::drawCurvedPanel(const float* pv, float x, float y, float z,
                                    float width, float height, float curveDepth, GLuint texture,
                                    const std::array<float,4>& color, int segments) {
    segments = std::clamp(segments, 8, 48);
    std::vector<Vertex> vertices;
    vertices.reserve(static_cast<size_t>(segments) * 6u);
    const float pitchCos = std::cos(kMonitorScreenPitch);
    const float pitchSin = std::sin(kMonitorScreenPitch);
    auto point = [&](float u, float v) {
        const float localX = (u - 0.5f) * width;
        const float normalized = 2.0f * u - 1.0f;
        // Match the monitor glass: shallow concave center, with both edges projecting forward.
        const float localZ = curveDepth * normalized * normalized;
        const float localY = (v - 0.5f) * height;
        const float pitchedY = pitchCos * localY + pitchSin * localZ;
        const float pitchedZ = -pitchSin * localY + pitchCos * localZ;
        return Vertex{localX, pitchedY, pitchedZ, u, 1.0f - v};
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
    glUniformMatrix4fv(locModel_,1,GL_FALSE,model.data());
    glUniform4fv(locColor_, 1, color.data());
    glUniform1i(locTextured_, texture ? 1 : 0);
    glUniform1i(locMaterial_,texture ? -1 : 4);
    glUniform1i(locRounded_,0);
    glUniform2f(locUvScale_,1.0f,1.0f);
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

void RoomRenderer::drawGroundShadow(const float* pv,float x,float y,float z,
                                     float width,float depth,float opacity) {
    if(!groundShadowTexture_) return;
    Mat4 projectionView{};std::copy(pv,pv+projectionView.size(),projectionView.begin());
    const Mat4 model=mul(translation(x,y,z),mul(rotateX(-kPi*0.5f),scaling(width,depth,1.0f)));
    const Mat4 mvp=mul(projectionView,model);
    glUniformMatrix4fv(locMvp_,1,GL_FALSE,mvp.data());
    glUniformMatrix4fv(locModel_,1,GL_FALSE,model.data());
    glUniform4f(locColor_,1.0f,1.0f,1.0f,std::clamp(opacity,0.0f,1.0f));
    glUniform1i(locTextured_,1);glUniform1i(locMaterial_,-1);glUniform1i(locRounded_,0);
    glUniform2f(locUvScale_,1.0f,1.0f);
    glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,groundShadowTexture_);
    glUniform1i(locTexture_,0);
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glBindVertexArray(planeTexture_);glDrawArrays(GL_TRIANGLES,0,6);glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D,0);
}
void RoomRenderer::drawEnvironmentWindow(const float* pv,float wallX,float centerZ,float floorY,
                                         GLuint tex,float windowW,float windowH,float yaw) {
    const float windowY=floorY+2.4f;
    auto point=[&](float localX,float localY,float localZ) {
        const float co=std::cos(yaw),si=std::sin(yaw);
        return std::array<float,3>{wallX+co*localX+si*localZ,localY,
                                   centerZ-si*localX+co*localZ};
    };
    auto box=[&](float lx,float ly,float lz,float sx,float sy,float sz,
                 const std::array<float,4>& color,int material=0) {
        const auto p=point(lx,ly,lz);
        drawBox(pv,p[0],p[1],p[2],sx,sy,sz,color,yaw,material);
    };
    auto rounded=[&](float lx,float ly,float lz,float sx,float sy,float sz,
                     const std::array<float,4>& color,int material=0) {
        const auto p=point(lx,ly,lz);
        drawRoundedBox(pv,p[0],p[1],p[2],sx,sy,sz,color,yaw,material);
    };
    // Right-wall glazing sits beside the bed; local +Z faces into the room.
    const auto glass=point(0,windowY,0.035f);
    drawPlane(pv,glass[0],glass[1],glass[2],windowW,windowH,tex,rgb(1,1,1),yaw);
    const auto frame=rgb(.018f,.025f,.045f);
    const float t=0.062f;
    box(0,windowY+windowH*.5f+t*.5f,0.015f,windowW+2*t,t,0.12f,frame);
    box(0,windowY-windowH*.5f-t*.5f,0.015f,windowW+2*t,t,0.12f,frame);
    box(-windowW*.5f-t*.5f,windowY,0.015f,t,windowH,0.12f,frame);
    box(windowW*.5f+t*.5f,windowY,0.015f,t,windowH,0.12f,frame);
    box(0,windowY,0.005f,0.035f,windowH,0.10f,frame);
    // Divided-light sash, recessed glazing and a deep interior sill.
    const auto sash=rgb(.052f,.068f,.11f);
    const float sashZ=.092f;
    for(int pane=-1;pane<=1;++pane) {
        if(pane==0) continue;
        rounded(pane*windowW*.25f,windowY,sashZ,.026f,windowH-.055f,.065f,sash,1);
    }
    rounded(0,windowY+windowH*.12f,sashZ,windowW-.045f,.026f,.065f,sash,1);
    const float sillY=windowY-windowH*.5f-.065f;
    rounded(0,sillY,.12f,windowW+.24f,.085f,.25f,frame,5);
    rounded(0,sillY-.10f,.075f,windowW+.12f,.13f,.09f,sash,5);
    if(activeEnvironmentIndex_==29) {
        box(0,windowY+windowH*.5f+.018f,.086f,windowW*.96f,.009f,.012f,rgb(.12f,.30f,1.0f),7);
        box(0,windowY-windowH*.5f-.018f,.086f,windowW*.96f,.008f,.010f,rgb(.58f,.10f,1.0f),7);
    }
    const auto brass=rgb(.62f,.47f,.23f);
    for(int side : {-1,1}) {
        const auto p=point(side*(windowW*.5f-.10f),sillY+.075f,.225f);
        drawSphere(pv,p[0],p[1],p[2],.018f,.018f,.010f,brass,4);
    }
}

void RoomRenderer::updateLocomotion(float leftX,float leftY,float rightStickX,float dt,const XrPosef& headPose) {
    if(dt<0.0f||dt>0.1f)dt=0.0f;
    const float yaw=headYaw(headPose)+virtualYaw_;
    const float speed=std::clamp(settings_.movementSpeed,0.1f,4.0f);

    // Match makeView(): OpenXR local forward (-Z) rotates toward -X for +yaw.
    const float forwardX=-std::sin(yaw), forwardZ=-std::cos(yaw);
    const float rightX=std::cos(yaw), rightZ=-std::sin(yaw);
    const float stepX=settings_.roomMovementEnabled?(rightX*leftX+forwardX*leftY)*speed*dt:0.0f;
    const float stepZ=settings_.roomMovementEnabled?(rightZ*leftX+forwardZ*leftY)*speed*dt:0.0f;

    float halfW=3.0f,halfD=3.8f;
    if(settings_.roomSize=="large"){halfW=6.0f;halfD=7.5f;}
    else if(settings_.roomSize=="medium"){halfW=4.2f;halfD=5.4f;}

    // Collision checks run in room coordinates, not locomotion-offset-only coordinates.
    const float playerX=headPose.position.x+moveX_;
    const float playerZ=headPose.position.z+moveZ_;
    float targetX=std::clamp(playerX+stepX,-halfW+0.34f,halfW-0.34f);
    float targetZ=std::clamp(playerZ+stepZ,-halfD+0.34f,halfD-0.34f);

    const float deskX=-halfW+1.32f, deskZ=-halfD+0.43f;
    const float bedX=halfW-2.0f, bedZ=-halfD+2.10f;
    auto overlaps=[](float x,float z,float cx,float cz,float rx,float rz) {
        return std::fabs(x-cx)<rx && std::fabs(z-cz)<rz;
    };
    auto collides=[&](float x,float z) {
        return overlaps(x,z,deskX,deskZ,1.36f,.52f) ||
               overlaps(x,z,bedX,bedZ,1.30f,1.72f) ||
               overlaps(x,z,bedX-1.53f,bedZ-.94f,.52f,.54f) ||
               overlaps(x,z,bedX+1.53f,bedZ-.94f,.52f,.54f);
    };

    // Axis-separated resolution permits sliding alongside furniture.
    if(collides(targetX,playerZ) && !collides(playerX,playerZ)) targetX=playerX;
    if(collides(targetX,targetZ) && !collides(targetX,playerZ)) targetZ=playerZ;
    moveX_=targetX-headPose.position.x;
    moveZ_=targetZ-headPose.position.z;

    snapCooldown_=std::max(0.0f,snapCooldown_-dt);
    if(settings_.smoothTurn){
        // Positive right-stick input decreases yaw, matching snap-turn-right.
        virtualYaw_-=std::clamp(rightStickX,-1.0f,1.0f)*
                     std::clamp(settings_.turnSpeed,0.2f,3.5f)*dt;
    } else {
        const bool right=rightStickX>0.7f,left=rightStickX < -0.7f;
        if(snapCooldown_<=0.0f && right && !previousTurnRight_){
            virtualYaw_-=settings_.snapTurnDegrees*kPi/180.0f;
            snapCooldown_=0.24f;
        } else if(snapCooldown_<=0.0f && left && !previousTurnLeft_){
            virtualYaw_+=settings_.snapTurnDegrees*kPi/180.0f;
            snapCooldown_=0.24f;
        }
        previousTurnRight_=right;previousTurnLeft_=left;
    }
}

void RoomRenderer::handleRoomInteraction(const XrPosef& controllerPose, bool triggerPressed) {
    const bool triggerEdge = triggerPressed && !previousRoomTrigger_;
    previousRoomTrigger_ = triggerPressed;
    if (!triggerEdge || menuVisible_ || !settings_.roomControlsEnabled) return;

    float halfW=3.0f,halfD=3.8f;
    if(settings_.roomSize=="large"){halfW=6.0f;halfD=7.5f;}
    else if(settings_.roomSize=="medium"){halfW=4.2f;halfD=5.4f;}
    (void)halfW;

    const float floorY=settings_.floorOffsetY;
    const float frontZ=halfD;
    const float bedX=halfW-2.0f;
    const float bedZ=-halfD+2.10f;
    const float remoteX=bedX+1.73f;
    const float remoteZ=bedZ-.77f;

    const XrQuaternionf roomYaw{0.0f,std::sin(virtualYaw_*.5f),0.0f,std::cos(virtualYaw_*.5f)};
    const XrQuaternionf roomOrientation=quatMul(roomYaw,controllerPose.orientation);
    auto rotateVector=[](const XrQuaternionf& q,const std::array<float,3>& v) {
        const float tx=2.0f*(q.y*v[2]-q.z*v[1]);
        const float ty=2.0f*(q.z*v[0]-q.x*v[2]);
        const float tz=2.0f*(q.x*v[1]-q.y*v[0]);
        return std::array<float,3>{
            v[0]+q.w*tx+(q.y*tz-q.z*ty),
            v[1]+q.w*ty+(q.z*tx-q.x*tz),
            v[2]+q.w*tz+(q.x*ty-q.y*tx)
        };
    };
    const auto origin=rotateVector(roomYaw,{controllerPose.position.x+moveX_,
                                             controllerPose.position.y,
                                             controllerPose.position.z+moveZ_});
    auto direction=rotateVector(roomOrientation,{0.0f,0.0f,-1.0f});
    const float directionLength=std::sqrt(direction[0]*direction[0]+direction[1]*direction[1]+direction[2]*direction[2]);
    if(directionLength<0.0001f)return;
    for(float& component:direction)component/=directionLength;

    enum class Target { None, LightSwitch, LedRemote };
    Target target=Target::None;
    float nearest=4.5f;
    int selectedPreset=ledPreset_;

    // Wall switch: a forward-facing hit on the rocker beside the entry opening.
    constexpr float lightSwitchX=1.12f;
    const float lightSwitchY=floorY+1.42f;
    const float lightSwitchFaceZ=frontZ-.132f;
    if(direction[2]>0.10f) {
        const float t=(lightSwitchFaceZ-origin[2])/direction[2];
        if(t>0.0f&&t<nearest) {
            const float hitX=origin[0]+direction[0]*t;
            const float hitY=origin[1]+direction[1]*t;
            if(std::fabs(hitX-lightSwitchX)<=.145f&&std::fabs(hitY-lightSwitchY)<=.17f) {
                nearest=t;
                target=Target::LightSwitch;
            }
        }
    }

    // The small six-button remote sits on the right bedside table. Its button
    // surface is deliberately a little larger than the visible caps for VR use.
    if(direction[1]<-0.12f) {
        const float remoteTopY=floorY+.746f;
        const float t=(remoteTopY-origin[1])/direction[1];
        if(t>0.0f&&t<nearest) {
            const float hitX=origin[0]+direction[0]*t;
            const float hitZ=origin[2]+direction[2]*t;
            if(std::fabs(hitX-remoteX)<=.083f&&std::fabs(hitZ-remoteZ)<=.136f) {
                const int column=hitX<remoteX?0:1;
                const int row=std::clamp(static_cast<int>(std::lround((hitZ-remoteZ)/.060f))+1,0,2);
                nearest=t;
                selectedPreset=row*2+column;
                target=Target::LedRemote;
            }
        }
    }

    if(target==Target::LightSwitch) {
        roomLightsOn_=!roomLightsOn_;
        LOGI("Room lighting %s",roomLightsOn_?"on":"off");
    } else if(target==Target::LedRemote) {
        applyLedPreset(selectedPreset);
        LOGI("Room LED preset changed to %d",ledPreset_);
    }
}

XrPosef RoomRenderer::projectionPose(const XrPosef& viewPose) const {
    XrPosef adjusted=viewPose;
    const XrQuaternionf virtualTurn{0.0f,std::sin(virtualYaw_*.5f),0.0f,std::cos(virtualYaw_*.5f)};
    adjusted.orientation=quatMul(virtualTurn,viewPose.orientation);
    adjusted.position.x+=moveX_;
    adjusted.position.z+=moveZ_;
    return adjusted;
}

void RoomRenderer::renderEye(int eyeIndex,int width,int height,const XrView& view,GLuint videoTexture,bool hasVideo) {
    if(!initialized_)return;
    float halfW=3.0f,halfD=3.8f;
    if(settings_.roomSize=="large"){halfW=6.0f;halfD=7.5f;}else if(settings_.roomSize=="medium"){halfW=4.2f;halfD=5.4f;}
    (void)eyeIndex; (void)width; (void)height;
    const float floorY=settings_.floorOffsetY, ceilingY=floorY+4.4f, backZ=-halfD, frontZ=halfD;
    Mat4 projection=makeProjection(view.fov);
    Mat4 camera=makeView(view.pose,moveX_,moveZ_,virtualYaw_);
    Mat4 pv=mul(projection,camera);
    // Recover the camera position from the exact view matrix already used for this eye.
    // This only feeds material view-dependent highlights; it does not alter the view transform.
    const float cameraX=-(camera[0]*camera[12]+camera[1]*camera[13]+camera[2]*camera[14]);
    const float cameraY=-(camera[4]*camera[12]+camera[5]*camera[13]+camera[6]*camera[14]);
    const float cameraZ=-(camera[8]*camera[12]+camera[9]*camera[13]+camera[10]*camera[14]);
    const auto& env=environmentTextures_[activeEnvironmentIndex_];
    const auto ledHeadboard=ledZoneAverageColor(0);
    const auto ledCeiling=ledZoneAverageColor(1);
    const auto ledLeftWall=ledZoneAverageColor(2);
    const auto ledRightWall=ledZoneAverageColor(3);
    const auto ledRear=ledZoneAverageColor(4);
    const auto ledLeftLamp=ledZoneAverageColor(5);
    const auto ledRightLamp=ledZoneAverageColor(6);
    const auto accentBlue=rgb(ledLeftWall[0],ledLeftWall[1],ledLeftWall[2]);
    const auto accentViolet=rgb(ledRightWall[0],ledRightWall[1],ledRightWall[2]);
    const auto neonCyan=rgb(ledRear[0],ledRear[1],ledRear[2]);
    const auto neonMagenta=rgb(ledRear[0],ledRear[1],ledRear[2]);
    auto drawGradientBar=[&](int zone,float x,float y,float z,float sx,float sy,float sz,int segments=24) {
        if(zone<0||zone>=static_cast<int>(ledZones_.size())||!ledZones_[static_cast<size_t>(zone)].enabled) return;
        const float segmentWidth=sx/static_cast<float>(segments);
        for(int segment=0;segment<segments;++segment) {
            const float t=(static_cast<float>(segment)+0.5f)/static_cast<float>(segments);
            const auto color=ledZoneColor(zone,t);
            drawBox(pv.data(),x-sx*.5f+segmentWidth*(segment+.5f),y,z,
                    segmentWidth*1.08f,sy,sz,rgb(color[0],color[1],color[2]),0.0f,7);
        }
    };
    auto drawGradientBarY=[&](int zone,float x,float y,float z,float sx,float sy,float sz,int segments=24) {
        if(zone<0||zone>=static_cast<int>(ledZones_.size())||!ledZones_[static_cast<size_t>(zone)].enabled) return;
        const float segmentHeight=sy/static_cast<float>(segments);
        for(int segment=0;segment<segments;++segment) {
            const float t=(static_cast<float>(segment)+0.5f)/static_cast<float>(segments);
            const auto color=ledZoneColor(zone,t);
            drawBox(pv.data(),x,y-sy*.5f+segmentHeight*(segment+.5f),z,
                    sx,segmentHeight*1.08f,sz,rgb(color[0],color[1],color[2]),0.0f,7);
        }
    };
    std::array<float,4> wall = rgb(.14f,.17f,.25f);
    std::array<float,4> floor = rgb(.09f,.12f,.18f);
    std::array<float,4> wood = rgb(.22f,.18f,.22f);
    std::array<float,4> upholstery = rgb(.19f,.24f,.33f);
    switch (activeEnvironmentIndex_) {
    case 0: wall=rgb(.70f,.63f,.67f); floor=rgb(.44f,.37f,.41f); wood=rgb(.32f,.20f,.21f); upholstery=rgb(.63f,.38f,.48f); break;
    case 1: wall=rgb(.10f,.13f,.20f); floor=rgb(.10f,.14f,.19f); wood=rgb(.12f,.14f,.17f); upholstery=rgb(.11f,.20f,.28f); break;
    case 2: wall=rgb(.20f,.15f,.12f); floor=rgb(.17f,.12f,.09f); wood=rgb(.30f,.18f,.09f); upholstery=rgb(.45f,.25f,.09f); break;
    case 3: wall=rgb(.07f,.10f,.18f); floor=rgb(.035f,.055f,.10f); wood=rgb(.11f,.10f,.21f); upholstery=rgb(.18f,.22f,.36f); break;
    case 4: case 17: case 18: case 19: case 24:
        wall=rgb(.09f,.055f,.16f); floor=rgb(.035f,.025f,.075f); wood=rgb(.12f,.045f,.20f); upholstery=rgb(.18f,.08f,.23f); break;
    case 5: case 8: case 14: case 16: case 25:
        wall=rgb(.34f,.21f,.26f); floor=rgb(.27f,.17f,.16f); wood=rgb(.33f,.20f,.12f); upholstery=rgb(.49f,.24f,.25f); break;
    case 6: case 11: case 20: case 27:
        wall=rgb(.12f,.22f,.31f); floor=rgb(.12f,.19f,.25f); wood=rgb(.19f,.26f,.30f); upholstery=rgb(.25f,.35f,.42f); break;
    case 7: case 21: case 26:
        wall=rgb(.07f,.19f,.16f); floor=rgb(.045f,.12f,.10f); wood=rgb(.13f,.20f,.13f); upholstery=rgb(.12f,.26f,.18f); break;
    case 9: case 13: case 15:
        wall=rgb(.055f,.20f,.26f); floor=rgb(.03f,.12f,.17f); wood=rgb(.10f,.23f,.23f); upholstery=rgb(.10f,.31f,.34f); break;
    case 10: case 23:
        wall=rgb(.06f,.06f,.17f); floor=rgb(.025f,.025f,.09f); wood=rgb(.13f,.09f,.24f); upholstery=rgb(.13f,.12f,.30f); break;
    case 12: case 22:
        wall=rgb(.22f,.10f,.09f); floor=rgb(.12f,.06f,.065f); wood=rgb(.22f,.12f,.085f); upholstery=rgb(.28f,.13f,.11f); break;
    default: break;
    }
    if (activeEnvironmentIndex_ >= 28) {
        switch ((activeEnvironmentIndex_ - 28) / 8) {
        case 0: wall=rgb(.10f,.12f,.22f); floor=rgb(.07f,.08f,.15f); wood=rgb(.10f,.11f,.22f); upholstery=rgb(.18f,.11f,.30f); break;
        case 1: wall=rgb(.10f,.20f,.15f); floor=rgb(.06f,.13f,.09f); wood=rgb(.13f,.21f,.13f); upholstery=rgb(.14f,.30f,.19f); break;
        case 2: wall=rgb(.06f,.19f,.24f); floor=rgb(.03f,.11f,.16f); wood=rgb(.08f,.19f,.23f); upholstery=rgb(.08f,.29f,.34f); break;
        case 3: wall=rgb(.12f,.20f,.29f); floor=rgb(.09f,.15f,.22f); wood=rgb(.16f,.25f,.31f); upholstery=rgb(.21f,.34f,.43f); break;
        case 4: wall=rgb(.08f,.07f,.20f); floor=rgb(.035f,.03f,.11f); wood=rgb(.12f,.07f,.25f); upholstery=rgb(.19f,.11f,.34f); break;
        case 5: wall=rgb(.25f,.13f,.10f); floor=rgb(.14f,.07f,.055f); wood=rgb(.24f,.12f,.07f); upholstery=rgb(.35f,.16f,.11f); break;
        case 6: wall=rgb(.24f,.18f,.25f); floor=rgb(.16f,.12f,.21f); wood=rgb(.28f,.19f,.22f); upholstery=rgb(.42f,.24f,.34f); break;
        default: wall=rgb(.11f,.08f,.25f); floor=rgb(.045f,.04f,.15f); wood=rgb(.15f,.09f,.28f); upholstery=rgb(.23f,.12f,.37f); break;
        }
    }
    if (activeEnvironmentIndex_ >= 88) {
        const auto palette = extraEnvironmentPalette(activeEnvironmentIndex_ - 28);
        wall = rgb(palette.skyHorizon[0] / 460.0f, palette.skyHorizon[1] / 460.0f, palette.skyHorizon[2] / 460.0f);
        floor = rgb(palette.ground[0] / 510.0f, palette.ground[1] / 510.0f, palette.ground[2] / 510.0f);
        wood = rgb(palette.skyTop[0] / 420.0f, palette.skyTop[1] / 420.0f, palette.skyTop[2] / 420.0f);
        upholstery = rgb(palette.skyHorizon[0] / 400.0f, palette.skyHorizon[1] / 400.0f, palette.skyHorizon[2] / 400.0f);
    }
    // Neutral, physically plausible finishes keep the room interior grounded;
    // selected scenes remain visible through the window and in practical accent lights.
    // Neon-noir material palette: dark charcoal and midnight-blue surfaces preserve contrast.
    wall=rgb(.075f,.083f,.135f);
    floor=rgb(.046f,.055f,.092f);
    wood=rgb(.075f,.080f,.125f);
    upholstery=rgb(.105f,.115f,.205f);
    glUseProgram(program_);glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);glDisable(GL_CULL_FACE);
    glUniform1i(locTexture_,0);
    glUniform3f(locCameraPosition_,cameraX,cameraY,cameraZ);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D,materialAtlasTexture_);
    glUniform1i(locMaterialAtlas_,1);
    glUniform1i(locHasMaterialAtlas_,materialAtlasTexture_?1:0);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D,normalAtlasTexture_);
    glUniform1i(locNormalAtlas_,2);
    glUniform1i(locHasNormalAtlas_,normalAtlasTexture_?1:0);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D,roughnessAtlasTexture_);
    glUniform1i(locRoughnessAtlas_,3);
    glUniform1i(locHasRoughnessAtlas_,roughnessAtlasTexture_?1:0);
    glActiveTexture(GL_TEXTURE0);
    const float roomLightScale=roomLightsOn_?1.0f:0.0f;
    glUniform1f(locRoomAmbientIntensity_,roomLightsOn_?1.0f:0.075f);
    glUniform3f(locCeilingLightPosition_,0.0f,ceilingY-0.14f,0.0f);
    glUniform3f(locCeilingLightColor_,1.0f,0.94f,0.84f);
    glUniform1f(locCeilingLightIntensity_,1.55f*roomLightScale);
    glUniform3f(locPracticalLightPosition_,-halfW+0.30f,floorY+1.92f,-1.45f);
    glUniform3f(locPracticalLightColor_,ledLeftLamp[0],ledLeftLamp[1],ledLeftLamp[2]);
    glUniform1f(locPracticalLightIntensity_,ledZones_[5].enabled?0.82f*roomLightScale:0.0f);
    glUniform3f(locPracticalLight2Position_,halfW-0.30f,floorY+1.92f,-1.45f);
    glUniform3f(locPracticalLight2Color_,ledRightLamp[0],ledRightLamp[1],ledRightLamp[2]);
    glUniform1f(locPracticalLight2Intensity_,ledZones_[6].enabled?0.82f*roomLightScale:0.0f);
    glUniform3f(locHeadboardGlowPosition_,halfW-2.0f,floorY+1.43f,-halfD+2.10f-1.22f);
    glUniform3f(locHeadboardGlowColor_,ledHeadboard[0],ledHeadboard[1],ledHeadboard[2]);
    glUniform1f(locHeadboardGlowIntensity_,ledZones_[0].enabled?2.60f*roomLightScale:0.0f);
    glUniform3f(locNeonBluePosition_,-halfW+0.20f,floorY+2.75f,-0.45f);
    glUniform3f(locNeonBlueColor_,ledLeftWall[0],ledLeftWall[1],ledLeftWall[2]);
    glUniform1f(locNeonBlueIntensity_,ledZones_[2].enabled?2.25f*roomLightScale:0.0f);
    glUniform3f(locNeonVioletPosition_,halfW-0.20f,floorY+2.55f,0.80f);
    glUniform3f(locNeonVioletColor_,ledRightWall[0],ledRightWall[1],ledRightWall[2]);
    glUniform1f(locNeonVioletIntensity_,ledZones_[3].enabled?2.05f*roomLightScale:0.0f);
    glUniform3f(locNeonPinkPosition_,0.0f,floorY+2.25f,backZ+0.58f);
    glUniform3f(locNeonPinkColor_,ledRear[0],ledRear[1],ledRear[2]);
    glUniform1f(locNeonPinkIntensity_,ledZones_[4].enabled?1.55f*roomLightScale:0.0f);
    // Room shell: floor, ceiling, side walls and back wall. Shapes stay anchored in world coordinates.
    // Deep polished stone replaces the old matte plank floor and picks up colored light.
    drawBox(pv.data(),0,floorY-0.08f,0,halfW*2+0.1f,0.16f,halfD*2+0.1f,floor,0.0f,8);
    drawBox(pv.data(),0,ceilingY+0.05f,0,halfW*2+0.1f,0.1f,halfD*2+0.1f,wall,0.0f,2);
    drawBox(pv.data(),-halfW-0.05f,(floorY+ceilingY)*.5f,0,0.1f,ceilingY-floorY,halfD*2,wall,0.0f,2);
    drawBox(pv.data(),halfW+0.05f,(floorY+ceilingY)*.5f,0,0.1f,ceilingY-floorY,halfD*2,wall,0.0f,2);
    drawBox(pv.data(),0,(floorY+ceilingY)*.5f,backZ-0.05f,halfW*2,ceilingY-floorY,0.1f,wall,0.0f,2);
    // Front wall has a centered doorway, so looking around still feels like being inside a room.
    const float sidePanelWidth = std::max(0.25f, (halfW * 2.0f - 1.4f) * 0.5f);
    drawBox(pv.data(),-(halfW+0.7f)*0.5f,(floorY+ceilingY)*0.5f,frontZ+0.02f,
            sidePanelWidth,ceilingY-floorY,0.10f,wall,0.0f,2);
    drawBox(pv.data(), (halfW+0.7f)*0.5f,(floorY+ceilingY)*0.5f,frontZ+0.02f,
            sidePanelWidth,ceilingY-floorY,0.10f,wall,0.0f,2);
    drawBox(pv.data(),0,ceilingY-1.05f,frontZ+0.02f,1.4f,2.1f,0.10f,wall,0.0f,2);

    // Finished entry opening: a real casing, timber leaf, inset panels, threshold and metal hardware.
    const float doorWidth=1.28f,doorHeight=2.24f,doorYaw=-0.70f;
    const float doorHingeX=.66f;
    const float doorX=doorHingeX-std::cos(doorYaw)*doorWidth*.5f;
    const float doorZ=frontZ-std::sin(-doorYaw)*doorWidth*.5f;
    const float doorY=floorY+doorHeight*.5f;
    const float casingZ=frontZ+.085f;
    const auto casingWood=rgb(wood[0]*1.18f,wood[1]*1.13f,wood[2]*1.08f);
    for(int side : {-1,1}) {
        drawRoundedBox(pv.data(),side*.765f,floorY+1.16f,casingZ,
                       .115f,2.42f,.15f,casingWood,0.0f,1);
        drawRoundedBox(pv.data(),side*.695f,floorY+1.14f,frontZ-.020f,
                       .055f,2.30f,.11f,rgb(.12f,.14f,.20f));
    }
    drawRoundedBox(pv.data(),0,floorY+2.365f,casingZ,1.62f,.14f,.15f,casingWood,0.0f,1);
    drawRoundedBox(pv.data(),0,floorY+.035f,frontZ-.045f,1.48f,.07f,.24f,
                   rgb(.34f,.28f,.22f),0.0f,1);
    // The leaf is deliberately ajar, so the opening still reads as a doorway rather than a blocked wall.
    drawRoundedBox(pv.data(),doorX,doorY,doorZ,doorWidth,doorHeight,.078f,wood,doorYaw,1);
    auto doorFacePoint=[&](float localX,float localY,float localZ) {
        return std::array<float,3>{
            doorX+std::cos(doorYaw)*localX+std::sin(doorYaw)*localZ,
            localY,
            doorZ-std::sin(doorYaw)*localX+std::cos(doorYaw)*localZ
        };
    };
    // Raised stiles and rails create two deep, framed timber panels on the interior face.
    const auto doorInset=rgb(wood[0]*.58f,wood[1]*.58f,wood[2]*.58f);
    for(int panel=0;panel<2;++panel) {
        const float panelY=floorY+(panel==0?1.68f:.61f);
        const auto p=doorFacePoint(0.0f,panelY,-.050f);
        drawRoundedBox(pv.data(),p[0],p[1],p[2],.80f,.76f,.018f,doorInset,doorYaw,1);
        for(int edge : {-1,1}) {
            const auto rail=doorFacePoint(edge*.455f,panelY,-.066f);
            drawRoundedBox(pv.data(),rail[0],rail[1],rail[2],.045f,.88f,.020f,casingWood,doorYaw,1);
        }
        for(int edge : {-1,1}) {
            const auto rail=doorFacePoint(0.0f,panelY+edge*.415f,-.066f);
            drawRoundedBox(pv.data(),rail[0],rail[1],rail[2],.88f,.045f,.020f,casingWood,doorYaw,1);
        }
    }
    // Three brass hinges and a backplate/lever set on the latch stile.
    const auto brass=rgb(.62f,.47f,.23f);
    for(int hinge=0;hinge<3;++hinge) {
        const float hy=floorY+.36f+hinge*.75f;
        const auto hp=doorFacePoint(.595f,hy,-.006f);
        drawRoundedBox(pv.data(),hp[0],hp[1],hp[2],.075f,.17f,.025f,brass,doorYaw,4);
        drawCylinder(pv.data(),hp[0],hp[1],hp[2]-.012f,.018f,.18f,brass,4);
    }
    const auto plate=doorFacePoint(-.48f,floorY+1.12f,-.067f);
    drawRoundedBox(pv.data(),plate[0],plate[1],plate[2],.085f,.18f,.018f,brass,doorYaw,4);
    const auto knob=doorFacePoint(-.48f,floorY+1.12f,-.105f);
    drawSphere(pv.data(),knob[0],knob[1],knob[2],.045f,.045f,.045f,brass,4);

    // A slim, flush-mounted smart rocker switch sits just to the right of the doorway.
    const float lightSwitchX=1.12f,lightSwitchY=floorY+1.42f;
    drawRoundedBox(pv.data(),lightSwitchX,lightSwitchY,frontZ-.082f,.19f,.31f,.038f,
                   rgb(.13f,.16f,.23f),0.0f,4);
    drawRoundedBox(pv.data(),lightSwitchX,lightSwitchY,frontZ-.105f,.158f,.276f,.012f,
                   rgb(.025f,.034f,.055f),0.0f,5);
    drawRoundedBox(pv.data(),lightSwitchX,lightSwitchY+(roomLightsOn_?.030f:-.030f),
                   frontZ-.125f,.064f,.132f,.014f,
                   roomLightsOn_?rgb(.29f,.34f,.43f):rgb(.105f,.125f,.17f),0.0f,4);
    drawSphere(pv.data(),lightSwitchX+.052f,lightSwitchY+.105f,frontZ-.133f,
               .010f,.010f,.010f,
               roomLightsOn_?rgb(1.0f,.58f,.16f):rgb(.045f,.055f,.075f),7);

    // A wide glazing bay occupies the right wall immediately beside the bed.
    const float roomWindowWidth=std::min(3.50f,halfD*.78f);
    const float roomWindowHeight=activeEnvironmentIndex_==1?2.55f:2.15f;
    // Place the glazing just inside the room's inner wall face so the solid wall cannot occlude it.
    const float windowWallX=halfW-.015f;
    const float windowCenterZ=-halfD+2.10f;
    const float windowYaw=-kPi*.5f;
    drawEnvironmentWindow(pv.data(),windowWallX,windowCenterZ,floorY,env,
                          roomWindowWidth,roomWindowHeight,windowYaw);

    // Curtains follow the same rotated plane and frame the bed-side window.
    const float curtainY=floorY+2.40f;
    const float curtainWidth=std::max(.18f,roomWindowWidth*.18f);
    const auto curtainFabric=activeEnvironmentIndex_==29?rgb(.035f,.043f,.075f):rgb(.43f,.40f,.37f);
    auto rightWallPoint=[&](float localX,float localY,float localZ) {
        return std::array<float,3>{windowWallX+std::sin(windowYaw)*localZ,localY,
                                   windowCenterZ-std::sin(windowYaw)*localX};
    };
    for(int side : {-1,1}) {
        const float centerLocalX=side*(roomWindowWidth*.5f+curtainWidth*.36f);
        const float panelWidth=curtainWidth*.76f;
        const float panelHeight=roomWindowHeight*1.18f;
        const auto center=rightWallPoint(centerLocalX,curtainY,.13f);
        drawCurtainPanel(pv.data(),center[0],center[1],center[2],
                         panelWidth,panelHeight,curtainFabric,windowYaw);
        const auto hem=rightWallPoint(centerLocalX,curtainY-panelHeight*.49f,.13f);
        drawRoundedBox(pv.data(),hem[0],hem[1],hem[2],panelWidth,.028f,.060f,
                       curtainFabric,windowYaw,3);
        const auto welt=rightWallPoint(centerLocalX-side*panelWidth*.49f,curtainY,.13f);
        drawRoundedBox(pv.data(),welt[0],welt[1],welt[2],.018f,panelHeight*.96f,.045f,
                       curtainFabric,windowYaw,3);
    }
    const auto rail=rightWallPoint(0,curtainY+roomWindowHeight*.59f,.13f);
    drawRoundedBox(pv.data(),rail[0],rail[1],rail[2],roomWindowWidth+curtainWidth*.9f,
                   .065f,.12f,wood,windowYaw,1);

    // A leafy floor plant and ceramic planter bring a natural material break to the back corner.
    const float plantX=-halfW+.62f, plantZ=frontZ-.44f;
    const auto potColor=rgb(.12f,.14f,.22f);
    const auto leafColor=rgb(.10f,.22f,.20f);
    drawCylinder(pv.data(),plantX,floorY+.24f,plantZ,.38f,.48f,potColor,5);
    drawRoundedBox(pv.data(),plantX,floorY+.49f,plantZ,.42f,.055f,.42f,rgb(.57f,.51f,.42f),0.0f,5);
    drawSphere(pv.data(),plantX,floorY+.53f,plantZ,.16f,.035f,.16f,rgb(.16f,.12f,.08f));
    for(int stem=0;stem<5;++stem) {
        const float a=2.0f*kPi*stem/5.0f;
        const float dx=std::cos(a)*(.12f+.025f*(stem%2));
        const float dz=std::sin(a)*(.12f+.025f*(stem%2));
        drawBox(pv.data(),plantX+dx*.45f,floorY+.83f,plantZ+dz*.45f,
                .018f,.62f,.018f,rgb(.16f,.31f,.20f),a);
        drawSphere(pv.data(),plantX+dx,floorY+1.04f,plantZ+dz,.14f,.22f,.075f,leafColor);
        drawSphere(pv.data(),plantX+dx*1.35f,floorY+.87f,plantZ+dz*1.35f,.18f,.085f,.075f,leafColor);
    }
    drawSphere(pv.data(),plantX,floorY+1.34f,plantZ,.20f,.19f,.10f,leafColor);
    drawSphere(pv.data(),plantX+.12f,floorY+1.26f,plantZ-.08f,.18f,.14f,.08f,leafColor);
    drawSphere(pv.data(),plantX-.12f,floorY+1.28f,plantZ+.06f,.17f,.16f,.08f,leafColor);

    const auto trimDark = rgb(.012f,.018f,.034f);
    const auto silver = rgb(.22f,.27f,.39f);

    // -------------------------------------------------------------------------
    // Penthouse bedroom: low upholstered platform, layered mattress, soft duvet,
    // separate pillows, tailored headboard, bedside storage and practical lamps.
    // Geometry stays inside the existing room bounds and is fully walkable around.
    // -------------------------------------------------------------------------
    const float bedX=halfW-2.0f;
    const float bedZ=backZ+2.10f;
    const auto bedBase=rgb(.018f,.024f,.043f);
    const auto bedMetal=rgb(.16f,.20f,.30f);

    // The complete bed is a custom, textured multi-part GLB built for this exact
    // bedroom anchor. The bedside cabinets and their controls remain native room props.
    drawGroundShadow(pv.data(),bedX,floorY+.004f,bedZ,3.20f,4.05f,.42f);
    const float bedPlacement[16] = {
        1,0,0,0, 0,1,0,0, 0,0,1,0, bedX,floorY,bedZ,1
    };
    bedModel_.draw(pv.data(),bedPlacement,locMvp_,locModel_,locColor_,locTextured_,
                   locTexture_,locMaterial_,locRounded_,locUvScale_);
    // Concealed headboard light: the upholstered bezel lips project in front
    // of this narrow source. A normal lying view sees the soft wall/bed bounce;
    // looking straight at the headboard reveals only the recessed light channel.
    if(ledZones_[0].enabled) {
        const float emitterZ=bedZ-1.135f;
        drawGradientBar(0,bedX,floorY+0.475f,emitterZ,2.14f,.012f,.010f,32);
        drawGradientBar(0,bedX,floorY+1.875f,emitterZ,2.14f,.012f,.010f,32);
        for(int edge=0;edge<2;++edge) {
            const float x=bedX+(edge==0?-1.075f:1.075f);
            drawGradientBarY(0,x,floorY+1.175f,emitterZ,.010f,1.40f,.012f,24);
        }
    }
    // Floating bedside tables with drawer fronts, recessed pulls and warm-free RGB task lamps.
    for(int side : {-1,1}) {
        const float nx=bedX+side*1.53f, nz=bedZ-.94f;
        drawRoundedBox(pv.data(),nx,floorY+.405f,nz,.52f,.60f,.56f,bedBase,0.0f,5);
        drawRoundedBox(pv.data(),nx,floorY+.68f,nz,.56f,.055f,.60f,bedMetal,0.0f,4);
        for(int drawer=0;drawer<2;++drawer) {
            const float dy=floorY+.34f+drawer*.22f;
            drawRoundedBox(pv.data(),nx,dy,nz+.289f,.445f,.185f,.025f,
                           rgb(.055f,.068f,.11f),0.0f,5);
            drawRoundedBox(pv.data(),nx,dy+.035f,nz+.308f,.10f,.012f,.012f,
                           bedMetal,0.0f,4);
        }
        drawCylinder(pv.data(),nx,floorY+.79f,nz,.055f,.20f,bedMetal,4);
        drawCylinder(pv.data(),nx,floorY+.925f,nz,.25f,.20f,
                     side<0?rgb(ledLeftLamp[0],ledLeftLamp[1],ledLeftLamp[2]):rgb(ledRightLamp[0],ledRightLamp[1],ledRightLamp[2]),3);
        drawCylinder(pv.data(),nx,floorY+1.035f,nz,.17f,.025f,
                     side<0?rgb(ledLeftLamp[0],ledLeftLamp[1],ledLeftLamp[2]):rgb(ledRightLamp[0],ledRightLamp[1],ledRightLamp[2]),0,0.0f);
        drawSphere(pv.data(),nx,floorY+.91f,nz,.07f,.075f,.07f,
                   side<0?rgb(ledLeftLamp[0],ledLeftLamp[1],ledLeftLamp[2]):rgb(ledRightLamp[0],ledRightLamp[1],ledRightLamp[2]),7);
        if(side>0) {
            const float remoteX=nx+.20f,remoteZ=nz+.17f,remoteY=floorY+.720f;
            drawGroundShadow(pv.data(),remoteX,floorY+.709f,remoteZ,.17f,.27f,.18f);
            drawRoundedBox(pv.data(),remoteX,remoteY,remoteZ,.15f,.026f,.26f,
                           rgb(.035f,.043f,.064f),0.0f,5);
            drawRoundedBox(pv.data(),remoteX,remoteY+.015f,remoteZ,.134f,.006f,.238f,
                           rgb(.075f,.092f,.135f),0.0f,4);
            const std::array<std::array<float,3>,6> swatches{{
                {{1.0f,.08f,.055f}},{{.04f,.95f,.16f}},
                {{.08f,.34f,1.0f}},{{.88f,.94f,1.0f}},
                {{.62f,.08f,1.0f}},{{.02f,.86f,.90f}}
            }};
            for(int row=0;row<3;++row) for(int column=0;column<2;++column) {
                const float bx=remoteX+(column==0?-.033f:.033f);
                const float bz=remoteZ+(row-1)*.060f;
                drawCylinder(pv.data(),bx,remoteY+.023f,bz,.040f,.008f,
                             rgb(.018f,.024f,.040f),4);
                const auto& swatch=swatches[row*2+column];
                drawCylinder(pv.data(),bx,remoteY+.029f,bz,.032f,.008f,
                             rgb(swatch[0],swatch[1],swatch[2]),7);
            }
            drawSphere(pv.data(),remoteX,remoteY+.031f,remoteZ-.102f,.010f,.006f,.010f,
                       ledPreset_==2?rgb(.08f,.78f,1.0f):rgb(.72f,.78f,.92f),7);
        }
    }

    // Place the supplied desk almost flush to both walls in the back-left corner.
    const float deskZ=backZ+0.43f;
    const float deskX=-halfW+1.32f;
    const int monitorCount=std::clamp(settings_.monitorCount,1,3);
    const float monitorBaseX=deskX-(monitorCount-1)*.335f;
    const float monitorX=monitorCount==3?deskX:monitorBaseX;
    const float deskSurface=floorY+.75f;
    const float monitorZ=deskZ-0.31f; // display stand sits at the rear edge, close to the wall

    // Soft contact shadows sit beneath the actual desk and relocated plant only.
    drawGroundShadow(pv.data(),deskX,floorY+0.004f,deskZ,2.70f,1.02f,.38f);
    drawGroundShadow(pv.data(),plantX,floorY+0.004f,plantZ,.62f,.64f,.24f);

    const float trimY = floorY + 3.72f;
    // Recessed RGB coves sit inside a dark ceiling reveal; the cores are truly emissive.
    drawRoundedBox(pv.data(),0,trimY,0,halfW*1.66f,.050f,.075f,trimDark,0.0f,5);
    drawGradientBar(1,0,trimY-.030f,0,halfW*1.53f,.012f,.022f,32);
    // Individually controlled left/right and rear room accents.
    for(int segment=0;segment<24;++segment) {
        const float z=-halfD*.82f+(segment+.5f)*(halfD*1.64f/24.0f);
        const auto left=ledZoneColor(2,(segment+.5f)/24.0f);
        const auto right=ledZoneColor(3,(segment+.5f)/24.0f);
        drawBox(pv.data(),-halfW+.12f,trimY-.08f,z,.020f,.022f,halfD*1.65f/24.0f,
                rgb(left[0],left[1],left[2]),0.0f,7);
        drawBox(pv.data(),halfW-.12f,trimY-.08f,z,.020f,.022f,halfD*1.65f/24.0f,
                rgb(right[0],right[1],right[2]),0.0f,7);
    }
    drawGradientBar(4,0,trimY-.08f,-halfD+.14f,halfW*1.60f,.020f,.022f,32);
    // Vertical bars are recessed into the side-wall reveals, rather than floating props.
    for(int side : {-1,1}) {
        for(int bar=0;bar<3;++bar) {
            const float z=-1.8f+bar*1.65f;
            const int zone=side<0?2:3;
            drawBox(pv.data(),side*(halfW-.16f),floorY+2.45f,z,.055f,1.28f,.10f,trimDark,0.0f,5);
            for(int segment=0;segment<12;++segment) {
                const float t=(segment+.5f)/12.0f;
                const auto tint=ledZoneColor(zone,t);
                drawBox(pv.data(),side*(halfW-.125f),floorY+2.45f-.91f*.5f+.91f*t,z,
                        .010f,.91f/12.0f*1.08f,.026f,
                        rgb(tint[0],tint[1],tint[2]),0.0f,7);
            }
        }
    }

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
    const auto wallTrim=rgb(.018f,.026f,.052f);
    const auto wallHighlight=rgb(.055f,.075f,.14f);
    const auto floorInlay=rgb(.025f,.045f,.085f);
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
        drawBox(pv.data(),lx,ceilingY-.012f,lz,.34f,.012f,.12f,wallTrim);
        drawBox(pv.data(),lx,ceilingY-.020f,lz,.25f,.006f,.075f,
                row==0?neonCyan:accentViolet,0.0f,7);
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

    // Framed abstract wall art on the open right wall, above the living-area sightline.
    const float artX=-halfW+.085f, artY=floorY+2.52f, artZ=1.75f;
    drawRoundedBox(pv.data(),artX,artY,artZ,.065f,1.04f,1.48f,trimDark,0.0f,5);
    drawBox(pv.data(),artX-.037f,artY,artZ,.012f,.94f,1.38f,rgb(.035f,.055f,.105f));
    // Glowing signal-line wall art in smoked glass, echoing the room's RGB palette.
    for(int trace=0;trace<3;++trace) {
        const float yy=artY+.28f-trace*.28f;
        const float zz=artZ-.44f+trace*.19f;
        drawBox(pv.data(),artX-.046f,yy,zz,.010f,.018f,.52f,trace==1?neonMagenta:neonCyan,0.0f,7);
        drawBox(pv.data(),artX-.046f,yy-.08f,zz+.24f,.010f,.16f,.018f,trace==1?neonMagenta:accentViolet,0.0f,7);
        drawSphere(pv.data(),artX-.052f,yy,zz-.25f,.012f,.034f,.034f,trace==1?neonMagenta:neonCyan,7);
    }

    // Draw only the supplied desk and monitor assets. The retired procedural
    // monitor shell and oversized duplicate screen are intentionally not rendered.
    const float deskPlacement[16] = {
        1,0,0,0, 0,1,0,0, 0,0,1,0, deskX,floorY,deskZ,1
    };
    deskModel_.draw(pv.data(),deskPlacement,locMvp_,locModel_,locColor_,locTextured_,
                    locTexture_,locMaterial_,locRounded_,locUvScale_);

    // Place up to three identical 27-inch screens across the supplied corner desk.
    // The central display remains the primary menu surface when a three-screen row is used.
    const bool streamOn=settings_.screenDestination=="monitor"||
                        settings_.screenDestination=="both";
    for(int screen=0;screen<monitorCount;++screen) {
        const float screenX=monitorBaseX+screen*.67f;
        const float monitorPlacement[16] = {
            1,0,0,0, 0,1,0,0, 0,0,1,0, screenX,deskSurface,monitorZ,1
        };
        monitorModel_.draw(pv.data(),monitorPlacement,locMvp_,locModel_,locColor_,locTextured_,
                           locTexture_,locMaterial_,locRounded_,locUvScale_);
        if(streamOn) {
            drawCurvedPanel(pv.data(),screenX,deskSurface+.280f,monitorZ+.001f,
                            .584f,.328f,.029f,
                            hasVideo?videoTexture:videoFallbackTexture_,rgb(1,1,1),32);
        }
    }

    // Bedroom TV centered beyond the foot of the bed and facing back toward it.
    // A compact floor-standing media console replaces the old living-area furniture.
    if(settings_.showTv) {
        const float tvX=bedX, tvZ=bedZ+3.18f;
        const float tvY=floorY+1.95f, tvW=std::min(2.35f,halfW*.82f), tvH=tvW*9.0f/16.0f;
        const bool tvStream=settings_.screenDestination=="tv"||settings_.screenDestination=="both";
        const GLuint tvTex=(tvStream&&hasVideo)?videoTexture:env;
        drawRoundedBox(pv.data(),tvX,tvY,tvZ,tvW+.14f,tvH+.14f,.085f,trimDark,0.0f,5);
        drawPlane(pv.data(),tvX,tvY,tvZ-.046f,tvW,tvH,tvTex,rgb(1,1,1),kPi);
        drawBox(pv.data(),tvX,tvY-tvH*.5f-.055f,tvZ-.047f,tvW*.54f,.010f,.006f,accentBlue,0.0f,7);
        const float cabinetY=floorY+.30f, cabinetZ=tvZ+.27f, cabinetW=tvW*.88f;
        drawGroundShadow(pv.data(),tvX,floorY+.004f,cabinetZ,.78f,2.15f,.28f);
        drawRoundedBox(pv.data(),tvX,cabinetY,cabinetZ,cabinetW,.50f,.50f,trimDark,0.0f,5);
        drawRoundedBox(pv.data(),tvX,cabinetY+.045f,cabinetZ-.238f,cabinetW-.08f,.39f,.018f,
                       rgb(.045f,.060f,.095f),0.0f,5);
        for(int bay=0;bay<3;++bay) {
            const float panelX=tvX+(bay-1)*cabinetW*.29f;
            drawRoundedBox(pv.data(),panelX,cabinetY+.045f,cabinetZ-.250f,
                           cabinetW*.255f,.35f,.008f,rgb(.055f,.070f,.115f),0.0f,5);
            drawBox(pv.data(),panelX,cabinetY+.045f,cabinetZ-.256f,
                    .12f,.006f,.004f,bay==1?accentViolet:accentBlue,0.0f,7);
        }
        drawRoundedBox(pv.data(),tvX,cabinetY-.265f,cabinetZ,.10f,.035f,.40f,silver,0.0f,4);
        for(int side : {-1,1}) {
            const float supportX=tvX+side*tvW*.22f;
            drawBox(pv.data(),supportX,floorY+.91f,tvZ+.085f,.035f,.78f,.045f,silver);
            drawBox(pv.data(),supportX,floorY+.55f,tvZ+.085f,.14f,.035f,.34f,trimDark);
        }
        drawBox(pv.data(),tvX,cabinetY+.262f,cabinetZ,.72f,.012f,.38f,accentBlue,0.0f,7);
    }

    const float lampX=-halfW+.30f,lampZ=-1.45f;
    drawCylinder(pv.data(),lampX,floorY+.055f,lampZ,.34f,.11f,trimDark,5);
    drawCylinder(pv.data(),lampX,floorY+.88f,lampZ,.038f,1.60f,silver,4);
    drawCylinder(pv.data(),lampX,floorY+1.70f,lampZ,.105f,.11f,wood,1);
    const auto lampGlow=activeEnvironmentIndex_==0?rgb(.98f,.72f,.82f):
        (activeEnvironmentIndex_==3||activeEnvironmentIndex_==26)?rgb(.30f,1.0f,.70f):
        (activeEnvironmentIndex_==4||activeEnvironmentIndex_==10||activeEnvironmentIndex_==24)?rgb(.25f,.72f,1.0f):
        (activeEnvironmentIndex_==16||activeEnvironmentIndex_==19||activeEnvironmentIndex_==25)?rgb(1.0f,.53f,.82f):rgb(1.0f,.63f,.30f);
    drawCylinder(pv.data(),lampX,floorY+1.91f,lampZ,.43f,.42f,
                 rgb(.72f,.68f,.60f),3,kPi);
    drawCylinder(pv.data(),lampX,floorY+1.70f,lampZ,.255f,.035f,lampGlow,5);
    drawCylinder(pv.data(),lampX,floorY+2.115f,lampZ,.18f,.035f,wood,1);
    drawSphere(pv.data(),lampX,floorY+1.84f,lampZ,.12f,.09f,.12f,lampGlow);
    // A fine pull cord gives the shade a scale reference and subtle vertical detail.
    drawCylinder(pv.data(),lampX+.17f,floorY+1.90f,lampZ,.009f,.30f,brass,4);
    if(settings_.comfortVignette && vignetteProgram_){
        glDisable(GL_DEPTH_TEST);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(vignetteProgram_);glBindVertexArray(planeTexture_);glDrawArrays(GL_TRIANGLES,0,6);glBindVertexArray(0);
        glUseProgram(0);glDisable(GL_BLEND);glEnable(GL_DEPTH_TEST);
    }
    // Full monitor UI with persistent navigation and distinct virtual-desktop-style pages.
    if (menuVisible_) {
        const float uiCurve=.029f,uiWidth=.574f,uiHeight=.322f;
        const float menuY=deskSurface+.280f,menuZ=monitorZ+.001f;
        const float navX=monitorX-.216f,bodyX=monitorX+.063f,bodyWidth=.405f;
        const auto background=rgb(.010f,.015f,.030f,.985f);
        const auto header=rgb(.018f,.028f,.052f,.99f);
        const auto navBase=rgb(.018f,.026f,.048f,.98f);
        const auto card=rgb(.035f,.050f,.082f,.98f);
        const auto selected=rgb(.045f,.145f,.265f,1.0f);
        const auto blue=rgb(.20f,.61f,1.0f,1.0f);
        const auto cyan=rgb(.20f,.82f,1.0f,1.0f);
        const auto white=rgb(.91f,.94f,1.0f,1.0f);
        const auto muted=rgb(.48f,.56f,.70f,1.0f);
        const auto green=rgb(.24f,.88f,.62f,1.0f);
        const auto amber=rgb(1.0f,.68f,.28f,1.0f);
        auto panel=[&](float x,float y,float w,float h,const std::array<float,4>& tint) {
            drawCurvedPanel(pv.data(),x,y,menuZ,w,h,uiCurve,0,tint,32);
        };
        auto label=[&](const char* value,float x,float y,float size,const std::array<float,4>& tint) {
            drawText(pv.data(),value,x,y,menuZ+.0015f,size,tint,monitorX,uiWidth,uiCurve,menuY);
        };
        auto prettyName=[](const std::string& value) {
            std::string out=value;
            std::replace(out.begin(),out.end(),'_',' ');
            std::transform(out.begin(),out.end(),out.begin(),
                           [](unsigned char ch){return static_cast<char>(std::toupper(ch));});
            return out;
        };
        auto drawRow=[&](int index,float y,const char* title,const std::string& value) {
            panel(bodyX,y,bodyWidth,.040f,menuSelection_==index?selected:card);
            label(title,bodyX-.184f,y+.005f,.00185f,white);
            label(value.c_str(),bodyX+.045f,y+.005f,.00175f,blue);
        };

        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(program_);
        panel(monitorX,menuY,uiWidth,uiHeight,background);
        panel(monitorX,menuY+.139f,uiWidth-.010f,.038f,header);
        label("QUESTBRIDGE",monitorX-.270f,menuY+.132f,.00245f,white);
        label("XR STREAMING",monitorX-.122f,menuY+.132f,.00155f,muted);
        label(pcConnected_?"PC ONLINE":"PC OFFLINE",monitorX+.130f,menuY+.132f,.00160f,
              pcConnected_?green:amber);
        panel(navX,menuY-.006f,.132f,.270f,navBase);
        panel(monitorX-.143f,menuY-.006f,.0025f,.270f,rgb(.09f,.16f,.25f,1.0f));
        label("LIBRARY",navX-.050f,menuY+.105f,.00135f,muted);
        const char* tabs[5]={"COMPUTERS","ENVIRONMENT","DISPLAY","LIGHTING","COMFORT"};
        const float tabY[5]={menuY+.061f,menuY+.015f,menuY-.031f,menuY-.077f,menuY-.123f};
        for(int i=0;i<5;++i) {
            panel(navX,tabY[i],.119f,.037f,i==menuPage_?selected:navBase);
            if(i==menuPage_) panel(navX-.055f,tabY[i],.004f,.024f,blue);
            label(tabs[i],navX-.048f,tabY[i]-.001f,.00142f,i==menuPage_?white:muted);
        }
        label("LEFT STICK  NAVIGATE",navX-.050f,menuY-.135f,.00105f,muted);
        const float contentTop=menuY+.092f;
        if(menuPage_==0) {
            label("COMPUTERS",bodyX-.184f,contentTop,.00205f,white);
            label("Connect to your Windows PC",bodyX-.184f,contentTop-.025f,.00145f,muted);
            panel(bodyX,menuY+.005f,bodyWidth,.105f,card);
            label("WINDOWS PC",bodyX-.178f,menuY+.038f,.00175f,white);
            label(pcHostIp_.c_str(),bodyX-.178f,menuY+.010f,.00165f,muted);
            label(pcConnected_?"CONNECTED":"WAITING FOR CONNECTION",bodyX+.025f,
                  menuY+.038f,.00155f,pcConnected_?green:amber);
            label(pcStreaming_?"VIDEO STREAM ACTIVE":"WAITING FOR VIDEO",bodyX+.025f,
                  menuY+.010f,.00135f,pcStreaming_?cyan:muted);
            panel(bodyX,menuY-.075f,bodyWidth,.045f,menuSelection_==0?selected:card);
            label("RECONNECT TO PC",bodyX-.175f,menuY-.080f,.00180f,white);
            label("A",bodyX+.165f,menuY-.080f,.00175f,blue);
            label("Check the PC IP in QuestBridge Studio if offline.",
                  bodyX-.184f,menuY-.125f,.00130f,muted);
        } else if(menuPage_==1) {
            label("ENVIRONMENT",bodyX-.184f,contentTop,.00205f,white);
            label("Change the room and how you move around it",bodyX-.184f,
                  contentTop-.024f,.00130f,muted);
            const std::string sizeValue=settings_.roomSize=="small"?"SMALL":
                (settings_.roomSize=="large"?"LARGE":"MEDIUM");
            char speed[24];std::snprintf(speed,sizeof(speed),"%.1f M/S",settings_.movementSpeed);
            drawRow(0,menuY+.034f,"ENVIRONMENT",prettyName(settings_.environment));
            drawRow(1,menuY-.010f,"ROOM SIZE",sizeValue);
            drawRow(2,menuY-.054f,"ROOM MOVEMENT",settings_.roomMovementEnabled?"ON":"OFF");
            drawRow(3,menuY-.098f,"MOVE SPEED",speed);
            drawRow(4,menuY-.142f,"RESET POSITION","RESET");
            if(environmentDropdownOpen_) {
                panel(bodyX+.050f,menuY-.044f,.298f,.158f,rgb(.014f,.024f,.044f,1.0f));
                label("CHOOSE ENVIRONMENT",bodyX-.080f,menuY+.020f,.00148f,cyan);
                for(int item=0;item<4;++item) {
                    const int index=(environmentDropdownIndex_+item)%
                        static_cast<int>(kEnvironmentNames.size());
                    const float yy=menuY-.012f-item*.031f;
                    panel(bodyX+.050f,yy,.282f,.027f,item==0?selected:card);
                    const std::string display=prettyName(kEnvironmentNames[index]);
                    label(display.c_str(),bodyX-.080f,yy-.004f,.00130f,item==0?white:muted);
                }
                label("A  APPLY",bodyX+.100f,menuY-.143f,.00125f,blue);
            }
        } else if(menuPage_==2) {
            label("DISPLAY",bodyX-.184f,contentTop,.00205f,white);
            label("Choose where your PC screen appears",bodyX-.184f,
                  contentTop-.024f,.00135f,muted);
            const std::string output=settings_.screenDestination=="tv"?"TV ONLY":
                (settings_.screenDestination=="both"?"MONITOR + TV":"MONITOR");
            char count[24];std::snprintf(count,sizeof(count),"%d",settings_.monitorCount);
            drawRow(0,menuY+.034f,"SCREEN OUTPUT",output);
            drawRow(1,menuY-.018f,"BEDROOM TV",settings_.showTv?"VISIBLE":"HIDDEN");
            drawRow(2,menuY-.070f,"VIRTUAL SCREENS",count);
            label("Your selected output updates the room display.",
                  bodyX-.184f,menuY-.132f,.00135f,muted);
        } else if(menuPage_==3) {
            const auto& zone=ledZones_[static_cast<size_t>(lightingZoneIndex_)];
            const char* zoneNames[7]={"HEADBOARD","CEILING COVE","LEFT WALL","RIGHT WALL","REAR ACCENT","LEFT BEDSIDE","RIGHT BEDSIDE"};
            label(lightingColorPickerOpen_?"COLOR PICKER":"ROOM LIGHTING",
                  bodyX-.184f,contentTop,.00205f,white);
            if(lightingColorPickerOpen_) {
                label(lightingColorTarget_==0?"PRIMARY COLOR":"SECONDARY COLOR",
                      bodyX-.184f,contentTop-.025f,.00132f,cyan);
                const float centerX=bodyX+.095f,centerY=menuY-.008f,radius=.060f;
                // A vivid hue ring with a saturation/value selector inside.
                for(int swatch=0;swatch<30;++swatch) {
                    const float hue=static_cast<float>(swatch)/30.0f;
                    const float angle=hue*2.0f*kPi-kPi*.5f;
                    const auto hueColor=hsvToRgb(hue,1.0f,1.0f);
                    panel(centerX+std::cos(angle)*radius,centerY+std::sin(angle)*radius,
                          .012f,.012f,rgb(hueColor[0],hueColor[1],hueColor[2]));
                }
                const float markerRadius=radius*(.12f+.74f*lightingPickerSaturation_);
                const float markerAngle=lightingPickerHue_*2.0f*kPi-kPi*.5f;
                const auto selectedColor=hsvToRgb(lightingPickerHue_,lightingPickerSaturation_,lightingPickerValue_);
                panel(centerX+std::cos(markerAngle)*markerRadius,
                      centerY+std::sin(markerAngle)*markerRadius,.017f,.017f,
                      rgb(selectedColor[0],selectedColor[1],selectedColor[2]));
                panel(centerX,centerY,.024f,.024f,
                      rgb(selectedColor[0],selectedColor[1],selectedColor[2]));
                char hueText[32],satText[32],valueText[32];
                std::snprintf(hueText,sizeof(hueText),"HUE  %03.0f",lightingPickerHue_*360.0f);
                std::snprintf(satText,sizeof(satText),"SAT  %02.0f%%",lightingPickerSaturation_*100.0f);
                std::snprintf(valueText,sizeof(valueText),"BRIGHT  %02.0f%%",lightingPickerValue_*100.0f);
                label(hueText,bodyX+.190f,menuY+.025f,.00138f,white);
                label(satText,bodyX+.190f,menuY-.006f,.00138f,muted);
                label(valueText,bodyX+.190f,menuY-.037f,.00138f,muted);
                label("LEFT STICK  HUE / SATURATION",bodyX-.184f,menuY-.105f,.00120f,muted);
                label("RIGHT STICK  BRIGHTNESS",bodyX-.184f,menuY-.130f,.00120f,muted);
                label("A  DONE",bodyX+.185f,menuY-.130f,.00120f,blue);
            } else {
                label("Choose a zone, then set solid color or a two-color gradient.",
                      bodyX-.184f,contentTop-.024f,.00122f,muted);
                const float rows[6]={menuY+.046f,menuY+.009f,menuY-.028f,menuY-.065f,menuY-.102f,menuY-.139f};
                const char* titles[6]={"LIGHT ZONE","COLOR MODE","PRIMARY COLOR","SECONDARY","BRIGHTNESS","ZONE POWER"};
                char brightness[24];std::snprintf(brightness,sizeof(brightness),"%d%%",static_cast<int>(zone.brightness*100.0f+.5f));
                const std::string values[6]={
                    zoneNames[lightingZoneIndex_],zone.gradient?"GRADIENT":"SOLID",
                    "EDIT COLOR",zone.gradient?"EDIT COLOR":"ONLY FOR GRADIENT",
                    brightness,zone.enabled?"ON":"OFF"
                };
                for(int row=0;row<6;++row) {
                    panel(bodyX,rows[row],bodyWidth,.032f,menuSelection_==row?selected:card);
                    label(titles[row],bodyX-.184f,rows[row]+.003f,.00148f,white);
                    label(values[row].c_str(),bodyX+.060f,rows[row]+.003f,.00140f,
                          row==5?(zone.enabled?green:amber):blue);
                }
                const auto primary=zone.primary;
                const auto secondary=zone.gradient?zone.secondary:zone.primary;
                panel(bodyX+.170f,rows[2],.018f,.018f,rgb(primary[0],primary[1],primary[2]));
                panel(bodyX+.170f,rows[3],.018f,.018f,rgb(secondary[0],secondary[1],secondary[2]));
                // A miniature left-to-right preview makes the selected blend obvious.
                for(int swatch=0;swatch<20;++swatch) {
                    const float t=(swatch+.5f)/20.0f;
                    const auto color=ledZoneColor(lightingZoneIndex_,t);
                    panel(bodyX-.085f+swatch*.0088f,menuY-.170f,.009f,.010f,
                          rgb(color[0],color[1],color[2]));
                }
            }
        } else {
            label("COMFORT",bodyX-.184f,contentTop,.00205f,white);
            label("Tune turning and room comfort",bodyX-.184f,
                  contentTop-.024f,.00135f,muted);
            char turnSpeed[24],snap[24];
            std::snprintf(turnSpeed,sizeof(turnSpeed),"%.1f",settings_.turnSpeed);
            std::snprintf(snap,sizeof(snap),"%.0f DEG",settings_.snapTurnDegrees);
            drawRow(0,menuY+.034f,"TURN MODE",settings_.smoothTurn?"SMOOTH":"SNAP");
            drawRow(1,menuY-.010f,"TURN SPEED",turnSpeed);
            drawRow(2,menuY-.054f,"SNAP ANGLE",snap);
            drawRow(3,menuY-.098f,"COMFORT VIGNETTE",settings_.comfortVignette?"ON":"OFF");
            label("Use the right stick to adjust highlighted values.",
                  bodyX-.184f,menuY-.145f,.00127f,muted);
        }
        if(!lightingColorPickerOpen_) {
            label("A  SELECT",monitorX-.270f,menuY-.151f,.00120f,muted);
            label("LEFT / RIGHT  TABS",monitorX-.145f,menuY-.151f,.00120f,muted);
        }
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
    }
    glBindTexture(GL_TEXTURE_2D,0);glBindVertexArray(0);glUseProgram(0);glDisable(GL_DEPTH_TEST);
}

void RoomRenderer::release() {
    bedModel_.release(); deskModel_.release(); monitorModel_.release();
    if(gFontAtlasTexture){glDeleteTextures(1,&gFontAtlasTexture);gFontAtlasTexture=0;gFontAtlasReady=false;}
    if(!initialized_&&!program_&&!vao_)return;
    if(program_)glDeleteProgram(program_);if(vignetteProgram_)glDeleteProgram(vignetteProgram_);if(vao_)glDeleteVertexArrays(1,&vao_);if(vbo_)glDeleteBuffers(1,&vbo_);
    if(planeTexture_)glDeleteVertexArrays(1,&planeTexture_);if(cubeTexture_)glDeleteBuffers(1,&cubeTexture_);
    if(textVao_)glDeleteVertexArrays(1,&textVao_);if(textVbo_)glDeleteBuffers(1,&textVbo_);
    if(curvedVao_)glDeleteVertexArrays(1,&curvedVao_);if(curvedVbo_)glDeleteBuffers(1,&curvedVbo_);
    if(sphereVao_)glDeleteVertexArrays(1,&sphereVao_);if(sphereVbo_)glDeleteBuffers(1,&sphereVbo_);
    if(roundedVao_)glDeleteVertexArrays(1,&roundedVao_);if(roundedVbo_)glDeleteBuffers(1,&roundedVbo_);
    if(pillowVao_)glDeleteVertexArrays(1,&pillowVao_);if(pillowVbo_)glDeleteBuffers(1,&pillowVbo_);
    if(curtainVao_)glDeleteVertexArrays(1,&curtainVao_);if(curtainVbo_)glDeleteBuffers(1,&curtainVbo_);
    if(cylinderVao_)glDeleteVertexArrays(1,&cylinderVao_);if(cylinderVbo_)glDeleteBuffers(1,&cylinderVbo_);
    for(auto t:environmentTextures_)if(t)glDeleteTextures(1,&t);if(videoFallbackTexture_)glDeleteTextures(1,&videoFallbackTexture_);
    if(groundShadowTexture_)glDeleteTextures(1,&groundShadowTexture_);
    if(materialAtlasTexture_)glDeleteTextures(1,&materialAtlasTexture_);
    if(normalAtlasTexture_)glDeleteTextures(1,&normalAtlasTexture_);
    if(roughnessAtlasTexture_)glDeleteTextures(1,&roughnessAtlasTexture_);
    program_=vignetteProgram_=vao_=vbo_=planeTexture_=cubeTexture_=videoFallbackTexture_=groundShadowTexture_=materialAtlasTexture_=normalAtlasTexture_=roughnessAtlasTexture_=sphereVao_=sphereVbo_=textVao_=textVbo_=curvedVao_=curvedVbo_=0;roundedVao_=roundedVbo_=pillowVao_=pillowVbo_=curtainVao_=curtainVbo_=cylinderVao_=cylinderVbo_=0;sphereVertexCount_=roundedVertexCount_=pillowVertexCount_=curtainVertexCount_=cylinderVertexCount_=0;environmentTextures_.fill(0);initialized_=false;
}
