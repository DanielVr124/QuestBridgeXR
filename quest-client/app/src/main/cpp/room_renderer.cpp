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
#include <zlib.h>

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
layout(location=2) in vec3 aNormal;
uniform mat4 uMvp;
uniform mat4 uModel;
uniform int uRounded;
uniform int uUseVertexNormal;
uniform vec3 uBevel;
out vec2 vUv;
out vec3 vWorldPosition;
out vec3 vVertexNormal;
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
  vVertexNormal=normalize(mat3(uModel)*aNormal);
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
in vec3 vVertexNormal;
uniform int uUseVertexNormal;
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
uniform vec3 uCeilingLightPosition;
uniform vec3 uCeilingLightColor;
uniform float uCeilingLightIntensity;
uniform vec3 uPracticalLightPosition;
uniform vec3 uPracticalLightColor;
uniform float uPracticalLightIntensity;
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
  vec3 normal=uUseVertexNormal!=0?normalize(vVertexNormal):normalize(cross(dpdx,dpdy));
  if (uUseVertexNormal==0 && !gl_FrontFacing) normal=-normal;
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
                                     base.rgb,roughness,metallic,vec3(.58,.60,.62));
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
  vec3 lit=ambient+directional+ceiling+practical+blueBounce+violetBounce+pinkBounce;
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


namespace {
bool readRoomAssetPayload(AAssetManager* assets,const char* filename,std::vector<uint8_t>& output) {
    output.clear();
    if(!assets||!filename) return false;
    const size_t nameLength=std::strlen(filename);
    const bool isParts=nameLength>=6&&std::strcmp(filename+nameLength-6,".parts")==0;
    const bool isBase64=isParts||(nameLength>=4&&std::strcmp(filename+nameLength-4,".b64")==0);
    std::vector<uint8_t> input;
    if(isParts) {
        const std::string base(filename,nameLength-6);
        for(int part=0;part<64;++part) {
            char suffix[16]{};
            std::snprintf(suffix,sizeof(suffix),".part%02d",part);
            const std::string partName=base+suffix;
            AAsset* asset=AAssetManager_open(assets,partName.c_str(),AASSET_MODE_BUFFER);
            if(!asset) {
                if(part==0) { LOGW("room asset parts missing: %s",filename); return false; }
                break;
            }
            const int64_t length=AAsset_getLength(asset);
            if(length<=0||length>20000||input.size()+static_cast<size_t>(length)>64u*1024u*1024u) {
                AAsset_close(asset);
                LOGW("invalid room asset part: %s",partName.c_str());
                return false;
            }
            const size_t offset=input.size();
            input.resize(offset+static_cast<size_t>(length));
            const int64_t read=AAsset_read(asset,input.data()+offset,static_cast<size_t>(length));
            AAsset_close(asset);
            if(read!=length) { LOGW("short room asset part: %s",partName.c_str()); return false; }
        }
        if(input.empty()) return false;
    } else {
        AAsset* asset=AAssetManager_open(assets,filename,AASSET_MODE_BUFFER);
        if(!asset) { LOGW("room asset missing: %s",filename); return false; }
        const int64_t length=AAsset_getLength(asset);
        if(length<=0||length>64LL*1024LL*1024LL) { AAsset_close(asset); LOGW("invalid room asset size: %s",filename); return false; }
        input.resize(static_cast<size_t>(length));
        const int64_t read=AAsset_read(asset,input.data(),static_cast<size_t>(length));
        AAsset_close(asset);
        if(read!=length) { LOGW("short room asset read: %s",filename); return false; }
    }

    if(isBase64) {
        std::vector<uint8_t> decoded;
        decoded.reserve(input.size()*3u/4u);
        uint32_t accumulator=0;
        int bits=-8;
        auto value=[](uint8_t ch)->int {
            if(ch>='A'&&ch<='Z') return ch-'A';
            if(ch>='a'&&ch<='z') return ch-'a'+26;
            if(ch>='0'&&ch<='9') return ch-'0'+52;
            if(ch=='+') return 62;
            if(ch=='/') return 63;
            return -1;
        };
        for(uint8_t ch:input) {
            if(ch=='=') break;
            const int v=value(ch);
            if(v<0) continue;
            accumulator=(accumulator<<6)|static_cast<uint32_t>(v);
            bits+=6;
            if(bits>=0) {
                decoded.push_back(static_cast<uint8_t>((accumulator>>bits)&0xffu));
                bits-=8;
                accumulator&=0x00ffffffu;
            }
        }
        input.swap(decoded);
    }
    if(input.size()>=8&&std::memcmp(input.data(),"QZB1",4)==0) {
        uint32_t expected=0;
        std::memcpy(&expected,input.data()+4,sizeof(expected));
        if(expected==0||expected>64u*1024u*1024u) { LOGW("bad compressed asset header: %s",filename); return false; }
        output.resize(expected);
        uLongf actual=static_cast<uLongf>(expected);
        const int status=uncompress(output.data(),&actual,input.data()+8,static_cast<uLong>(input.size()-8));
        if(status!=Z_OK||actual!=expected) {
            LOGW("room asset decompression failed (%d): %s",status,filename);
            output.clear();
            return false;
        }
        return true;
    }
    output.swap(input);
    return true;
}

GLuint RoomRenderer::loadRgbAsset(AAssetManager* assets,const char* filename) {
    std::vector<uint8_t> bytes;
    if(!readRoomAssetPayload(assets,filename,bytes)||bytes.size()<8) return 0;
    uint32_t w=0,h=0;
    std::memcpy(&w,bytes.data(),4);
    std::memcpy(&h,bytes.data()+4,4);
    const uint64_t expected=8ull+static_cast<uint64_t>(w)*h*3ull;
    if(w<2||h<2||w>2048||h>2048||expected>bytes.size()) {
        LOGW("bad RGB asset: %s",filename);
        return 0;
    }
    GLuint texture=0;
    glGenTextures(1,&texture);
    if(!texture) return 0;
    glBindTexture(GL_TEXTURE_2D,texture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,static_cast<GLsizei>(w),static_cast<GLsizei>(h),0,
                 GL_RGB,GL_UNSIGNED_BYTE,bytes.data()+8);
    glBindTexture(GL_TEXTURE_2D,0);
    LOGI("loaded %s (%ux%u)",filename,w,h);
    return texture;
}

bool RoomRenderer::loadModelAsset(AAssetManager* assets,const char* filename,std::vector<ModelMesh>& meshes) {
    for(auto& mesh:meshes) {
        if(mesh.vao) glDeleteVertexArrays(1,&mesh.vao);
        if(mesh.vbo) glDeleteBuffers(1,&mesh.vbo);
        if(mesh.ebo) glDeleteBuffers(1,&mesh.ebo);
    }
    meshes.clear();
    std::vector<uint8_t> bytes;
    if(!readRoomAssetPayload(assets,filename,bytes)||bytes.size()<8) return false;
    if(std::memcmp(bytes.data(),"QBM1",4)!=0) {
        LOGW("bad room model header: %s",filename);
        return false;
    }
    size_t cursor=4;
    uint32_t meshCount=0;
    std::memcpy(&meshCount,bytes.data()+cursor,4); cursor+=4;
    if(meshCount==0||meshCount>64) return false;

    auto read=[&](void* destination,size_t size)->bool {
        if(cursor>bytes.size()||size>bytes.size()-cursor) return false;
        std::memcpy(destination,bytes.data()+cursor,size);
        cursor+=size;
        return true;
    };
    for(uint32_t i=0;i<meshCount;++i) {
        int32_t material=5;
        uint32_t textureMode=0,vertexCount=0,indexCount=0;
        std::array<float,4> color{1.0f,1.0f,1.0f,1.0f};
        if(!read(&material,4)||!read(&textureMode,4)||!read(&vertexCount,4)||
           !read(&indexCount,4)||!read(color.data(),sizeof(float)*4)) {
            LOGW("truncated mesh header: %s",filename);
            return false;
        }
        if(vertexCount<3||vertexCount>250000||indexCount<3||indexCount>750000||
           (indexCount%3)!=0||textureMode>2) {
            LOGW("invalid mesh counts: %s",filename);
            return false;
        }
        const size_t vertexBytes=static_cast<size_t>(vertexCount)*8u*sizeof(float);
        const size_t indexBytes=static_cast<size_t>(indexCount)*sizeof(uint32_t);
        if(cursor>bytes.size()||vertexBytes>bytes.size()-cursor||
           indexBytes>bytes.size()-cursor-vertexBytes) {
            LOGW("mesh payload exceeds asset: %s",filename);
            return false;
        }
        ModelMesh mesh{};
        mesh.material=material;
        mesh.textureMode=static_cast<GLint>(textureMode);
        mesh.indexCount=static_cast<GLsizei>(indexCount);
        mesh.color=color;
        glGenVertexArrays(1,&mesh.vao);
        glGenBuffers(1,&mesh.vbo);
        glGenBuffers(1,&mesh.ebo);
        if(!mesh.vao||!mesh.vbo||!mesh.ebo) {
            if(mesh.vao) glDeleteVertexArrays(1,&mesh.vao);
            if(mesh.vbo) glDeleteBuffers(1,&mesh.vbo);
            if(mesh.ebo) glDeleteBuffers(1,&mesh.ebo);
            return false;
        }
        glBindVertexArray(mesh.vao);
        glBindBuffer(GL_ARRAY_BUFFER,mesh.vbo);
        glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(vertexBytes),bytes.data()+cursor,GL_STATIC_DRAW);
        cursor+=vertexBytes;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,mesh.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,static_cast<GLsizeiptr>(indexBytes),bytes.data()+cursor,GL_STATIC_DRAW);
        cursor+=indexBytes;
        constexpr GLsizei stride=8*static_cast<GLsizei>(sizeof(float));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,stride,reinterpret_cast<void*>(0));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,stride,reinterpret_cast<void*>(6*sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,stride,reinterpret_cast<void*>(3*sizeof(float)));
        glBindVertexArray(0);
        meshes.push_back(mesh);
    }
    glBindBuffer(GL_ARRAY_BUFFER,0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
    LOGI("loaded %s (%u mesh parts)",filename,meshCount);
    return true;
}

void RoomRenderer::drawModel(const float* pv,const std::vector<ModelMesh>& meshes,
                             float x,float y,float z,float scale,float yaw,
                             GLuint videoTexture,bool hasVideo) {
    if(meshes.empty()) return;
    Mat4 projectionView{};
    std::copy(pv,pv+projectionView.size(),projectionView.begin());
    const Mat4 model=mul(translation(x,y,z),mul(rotateY(yaw),scaling(scale,scale,scale)));
    const Mat4 mvp=mul(projectionView,model);
    glUniformMatrix4fv(locMvp_,1,GL_FALSE,mvp.data());
    glUniformMatrix4fv(locModel_,1,GL_FALSE,model.data());
    glUniform1i(locRounded_,0);
    glUniform2f(locUvScale_,1.0f,1.0f);
    glUniform1i(locUseVertexNormal_,1);
    for(const auto& mesh:meshes) {
        GLuint texture=0;
        const bool textured=mesh.textureMode!=0;
        if(mesh.textureMode==1) texture=deskSurfaceTexture_;
        else if(mesh.textureMode==2) {
            const bool streamOn=settings_.screenDestination=="monitor"||settings_.screenDestination=="both";
            texture=(streamOn&&hasVideo)?videoTexture:videoFallbackTexture_;
        }
        glUniform4fv(locColor_,1,mesh.color.data());
        glUniform1i(locTextured_,textured?1:0);
        glUniform1i(locMaterial_,textured?-1:mesh.material);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,texture);
        glUniform1i(locTexture_,0);
        glBindVertexArray(mesh.vao);
        glDrawElements(GL_TRIANGLES,mesh.indexCount,GL_UNSIGNED_INT,nullptr);
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D,0);
    glUniform1i(locUseVertexNormal_,0);
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
                        if (sceneIndex == 29) {
                            const float warmth = 0.72f + static_cast<float>((windowSeed >> 9) & 7U) * 0.04f;
                            r = 255.0f;
                            g = 166.0f + 48.0f * warmth;
                            b = 82.0f + 34.0f * warmth;
                        } else if (sceneIndex == 17) { r = 231.0f; g = 158.0f; b = 63.0f; }
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
                const float ripple = 0.5f + 0.5f * std::sin(v * 310.0f + u * 81.6814f)
                                            * std::sin(v * 91.0f - u * 43.9823f);
                r = 5.0f + depth * 6.0f + ripple * 8.0f;
                g = 7.0f + depth * 9.0f + ripple * 7.0f;
                b = 16.0f + depth * 15.0f + ripple * 7.0f;
                const int building = x / 16;
                const uint32_t seed = hash32(static_cast<uint32_t>(building) * 7919U
                                           + static_cast<uint32_t>(sceneIndex) * 113U + 37U);
                const int localX = x % 16;
                const bool litColumn = localX >= 4 && localX <= 10 && ((seed >> 5) % 7U < 3U);
                const int reflectionRow = static_cast<int>(depth * 92.0f + static_cast<float>((seed >> 12) & 15U));
                if (litColumn && (reflectionRow % 17) < 4) {
                    const float glow = (1.0f - depth * 0.72f) * (0.45f + ripple * 0.55f);
                    if (seed & 1U) { r += 190.0f * glow; g += 124.0f * glow; b += 52.0f * glow; }
                    else { r += 255.0f * glow; g += 177.0f * glow; b += 86.0f * glow; }
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
    locUseVertexNormal_=glGetUniformLocation(program_,"uUseVertexNormal");
    locCameraPosition_=glGetUniformLocation(program_,"uCameraPosition");
    locCeilingLightPosition_=glGetUniformLocation(program_,"uCeilingLightPosition");
    locCeilingLightColor_=glGetUniformLocation(program_,"uCeilingLightColor");
    locCeilingLightIntensity_=glGetUniformLocation(program_,"uCeilingLightIntensity");
    locPracticalLightPosition_=glGetUniformLocation(program_,"uPracticalLightPosition");
    locPracticalLightColor_=glGetUniformLocation(program_,"uPracticalLightColor");
    locPracticalLightIntensity_=glGetUniformLocation(program_,"uPracticalLightIntensity");
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
    environmentTextures_[0]=loadRgbAsset(assets,"environments/blossom.rgb");
    environmentTextures_[1]=loadRgbAsset(assets,"environments/night_city.rgb");
    environmentTextures_[2]=loadRgbAsset(assets,"environments/golden_city.rgb");
    materialAtlasTexture_=loadRgbAsset(assets,"materials/room_surface_atlas.rgb");
    normalAtlasTexture_=loadRgbAsset(assets,"materials/room_normal_atlas.rgb");
    roughnessAtlasTexture_=loadRgbAsset(assets,"materials/room_roughness_atlas.rgb");
    deskSurfaceTexture_=loadRgbAsset(assets,"models/arc_desk_texture.rgb.b64");
    environmentTextures_[29]=loadRgbAsset(assets,"environments/golden_city_360.rgb.b64");
    if(!loadModelAsset(assets,"models/arc_gaming_desk.qbm.parts",deskModel_))
        LOGW("Claude desk model failed to load; desk geometry will be unavailable.");
    if(!loadModelAsset(assets,"models/arc27_curved_monitor.qbm.parts",monitorModel_))
        LOGW("Claude ARC27 monitor failed to load; monitor geometry will be unavailable.");
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

    // Smooth UV sphere mesh used for chair hardware and PC cooling fans.
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
    const float deskX=-halfW+1.251f, deskZ=-halfD+0.422f;
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
                                      const std::array<float,4>& color) {
    Mat4 projectionView{};std::copy(pv,pv+projectionView.size(),projectionView.begin());
    const Mat4 model=mul(translation(x,y,z),scaling(width,height,1.0f));
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
    const Mat4 textModel=identity();
    glUniformMatrix4fv(locModel_,1,GL_FALSE,textModel.data());
    glUniform4fv(locColor_, 1, color.data());
    glUniform1i(locTextured_, 0);
    glUniform1i(locMaterial_,-1);
    glUniform1i(locRounded_,0);
    glBindVertexArray(textVao_);
    glBindBuffer(GL_ARRAY_BUFFER, textVbo_);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
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
void RoomRenderer::drawEnvironmentWindow(const float* pv,float halfW,float floorY,float backZ,
                                                     GLuint tex,float windowW,float windowH) {
    const float windowY=floorY+2.20f;
    const float halfD=std::fabs(backZ);
    const float frontZ=halfD;
    const auto drawPanoramaSegment=[&](float x,float y,float z,float width,float height,
                                       float yaw,float u0,float u1,const std::array<float,4>& tint) {
        if(!curvedVao_) {
            glGenVertexArrays(1,&curvedVao_);
            glGenBuffers(1,&curvedVbo_);
            glBindVertexArray(curvedVao_);
            glBindBuffer(GL_ARRAY_BUFFER,curvedVbo_);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),nullptr);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),
                                  reinterpret_cast<void*>(3*sizeof(float)));
            glBindVertexArray(0);
        }
        const Vertex bl{-0.5f,-0.5f,0.0f,u0,1.0f};
        const Vertex br{ 0.5f,-0.5f,0.0f,u1,1.0f};
        const Vertex tr{ 0.5f, 0.5f,0.0f,u1,0.0f};
        const Vertex tl{-0.5f, 0.5f,0.0f,u0,0.0f};
        const Vertex vertices[6]={bl,br,tr,bl,tr,tl};
        const Mat4 model=mul(translation(x,y,z),mul(rotateY(yaw),scaling(width,height,1.0f)));
        Mat4 projectionView{};
        std::copy(pv,pv+projectionView.size(),projectionView.begin());
        const Mat4 mvp=mul(projectionView,model);
        glUniformMatrix4fv(locMvp_,1,GL_FALSE,mvp.data());
        glUniformMatrix4fv(locModel_,1,GL_FALSE,model.data());
        glUniform4fv(locColor_,1,tint.data());
        glUniform1i(locTextured_,1);
        glUniform1i(locMaterial_,-1);
        glUniform1i(locRounded_,0);
        glUniform2f(locUvScale_,1.0f,1.0f);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,tex?tex:videoFallbackTexture_);
        glUniform1i(locTexture_,0);
        glBindVertexArray(curvedVao_);
        glBindBuffer(GL_ARRAY_BUFFER,curvedVbo_);
        glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLES,0,6);
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER,0);
        glBindTexture(GL_TEXTURE_2D,0);
    };

    // Panoramic glazing is confined to actual window openings. The front wall stays
    // opaque for the wall-mounted TV; solid wall remains visible above/below every pane.
    const float sideWindowLength=std::min(halfD*1.45f,5.2f);
    drawPanoramaSegment(0.0f,windowY,backZ+0.035f,windowW,windowH,0.0f,0.00f,0.25f,rgb(1,1,1));
    drawPanoramaSegment(halfW-0.035f,windowY,0.0f,sideWindowLength,windowH,-kPi*.5f,
                        0.25f,0.50f,rgb(1,1,1));
    drawPanoramaSegment(-halfW+0.035f,windowY,0.0f,sideWindowLength,windowH,kPi*.5f,
                        0.75f,1.00f,rgb(1,1,1));

    // A very light neutral glass pass supplies a restrained glazing sheen without obscuring
    // the city view. The image remains visible through the window openings.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    const auto glassTint=rgb(0.78f,0.88f,1.0f);
    auto glass=glassTint;
    glass[3]=0.045f;
    drawPanoramaSegment(0.0f,windowY,backZ+0.042f,windowW,windowH,0.0f,0.00f,0.25f,glass);
    drawPanoramaSegment(halfW-0.042f,windowY,0.0f,2.0f*halfD-0.16f,windowH,-kPi*.5f,
                        0.25f,0.50f,glass);
    drawPanoramaSegment(frontPaneCenter,windowY,frontZ-0.042f,frontPaneWidth,windowH,kPi,
                        0.50f,0.625f,glass);
    drawPanoramaSegment(-frontPaneCenter,windowY,frontZ-0.028f,frontPaneWidth,windowH,kPi,
                        0.625f,0.75f,glass);
    drawPanoramaSegment(-halfW+0.042f,windowY,0.0f,2.0f*halfD-0.16f,windowH,kPi*.5f,
                        0.75f,1.00f,glass);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    const auto frame=rgb(.018f,.025f,.045f);
    const float t=0.045f;
    // Minimal dark aluminium framing: clear glass dominates, no heavy center sash.
    drawBox(pv,0.0f,windowY+windowH*.5f,backZ+0.012f,windowW,t,0.10f,frame);
    drawBox(pv,0.0f,windowY-windowH*.5f,backZ+0.012f,windowW,t,0.10f,frame);
    drawBox(pv,-windowW*.5f,windowY,backZ+0.012f,t,windowH,0.10f,frame);
    drawBox(pv, windowW*.5f,windowY,backZ+0.012f,t,windowH,0.10f,frame);
    // Fine corner posts keep the wraparound glazing structurally believable.
    drawBox(pv,halfW-0.02f,windowY,backZ+0.012f,.055f,windowH,.10f,frame);
    drawBox(pv,halfW-0.02f,windowY,frontZ-0.012f,.055f,windowH,.10f,frame);
    drawBox(pv,-halfW+0.02f,windowY,backZ+0.012f,.055f,windowH,.10f,frame);
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
    const float deskX=-halfW+1.251f, deskZ=-halfD+0.422f;
    const float couchX=halfW-2.15f, couchZ=.55f;
    const bool hitsDesk=overlaps(moveX_,moveZ_,deskX,deskZ,1.45f,.70f);
    const bool hitsChair=settings_.showChair && overlaps(moveX_,moveZ_,deskX+.25f,deskZ+1.15f,.55f,.55f);
    const bool hitsCouch=settings_.showCouch && overlaps(moveX_,moveZ_,couchX,couchZ,.65f,1.20f);
    const float bedX=halfW-1.21f, bedZ=-halfD+1.60f;
    const bool hitsBed=overlaps(moveX_,moveZ_,bedX,bedZ,1.22f,1.55f);
    const bool hitsLeftNightstand=overlaps(moveX_,moveZ_,bedX-1.53f,bedZ-.94f,.34f,.34f);
    const bool hitsRightNightstand=false;
    if (hitsDesk || hitsChair || hitsCouch || hitsBed || hitsLeftNightstand || hitsRightNightstand) {
        moveX_ = oldMoveX; moveZ_ = oldMoveZ;
    }
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
    // Recover the camera position from the exact view matrix already used for this eye.
    // This only feeds material view-dependent highlights; it does not alter the view transform.
    const float cameraX=-(camera[0]*camera[12]+camera[1]*camera[13]+camera[2]*camera[14]);
    const float cameraY=-(camera[4]*camera[12]+camera[5]*camera[13]+camera[6]*camera[14]);
    const float cameraZ=-(camera[8]*camera[12]+camera[9]*camera[13]+camera[10]*camera[14]);
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
    // Neutral, physically plausible finishes keep the room interior grounded;
    // selected scenes remain visible through the window and in practical accent lights.
    // Neon-noir material palette: dark charcoal and midnight-blue surfaces preserve contrast.
    wall=rgb(.075f,.083f,.135f);
    floor=rgb(.046f,.055f,.092f);
    wood=rgb(.075f,.080f,.125f);
    sofa=rgb(.105f,.115f,.205f);
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
    glUniform3f(locCeilingLightPosition_,0.0f,ceilingY-0.14f,0.0f);
    glUniform3f(locCeilingLightColor_,1.0f,0.94f,0.84f);
    glUniform1f(locCeilingLightIntensity_,1.55f);
    glUniform3f(locPracticalLightPosition_,-halfW+0.30f,floorY+1.92f,-1.45f);
    glUniform3f(locPracticalLightColor_,0.40f,0.32f,1.0f);
    glUniform1f(locPracticalLightIntensity_,0.46f);
    // Recessed blue, violet and magenta fixtures cast colored bounce on nearby surfaces.
    glUniform3f(locNeonBluePosition_,-halfW+0.20f,floorY+2.75f,-0.45f);
    glUniform3f(locNeonBlueColor_,0.12f,0.34f,1.0f);
    glUniform1f(locNeonBlueIntensity_,2.25f);
    glUniform3f(locNeonVioletPosition_,halfW-0.20f,floorY+2.55f,0.80f);
    glUniform3f(locNeonVioletColor_,0.55f,0.14f,1.0f);
    glUniform1f(locNeonVioletIntensity_,2.05f);
    glUniform3f(locNeonPinkPosition_,0.0f,floorY+2.25f,backZ+0.58f);
    glUniform3f(locNeonPinkColor_,1.0f,0.075f,0.38f);
    glUniform1f(locNeonPinkIntensity_,1.55f);
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

    // The penthouse gets a near-wall-to-wall glazing system instead of a small picture window.
    const float roomWindowWidth=std::min(halfW*1.52f,
        settings_.roomSize=="small"?3.2f:(settings_.roomSize=="large"?7.2f:5.8f));
    const float roomWindowHeight=activeEnvironmentIndex_==29?2.45f:
        (activeEnvironmentIndex_==1?2.35f:activeEnvironmentIndex_>=3?2.15f:1.8f);
    drawEnvironmentWindow(pv.data(),halfW,floorY,backZ,env,roomWindowWidth,roomWindowHeight);

    // Tailored drapery softens the window opening: repeated fabric folds and a weighted hem.
    const float curtainY=floorY+2.40f;
    const float curtainZ=backZ+0.13f;
    const float curtainWidth=std::max(0.18f,roomWindowWidth*0.18f);
    const auto curtainFabric=activeEnvironmentIndex_==29?rgb(.035f,.043f,.075f):rgb(.43f,.40f,.37f);
    for(int side : {-1,1}) {
        const float curtainCenter=side*(roomWindowWidth*.5f+curtainWidth*.36f);
        const float curtainPanelWidth=curtainWidth*.76f;
        const float curtainHeight=roomWindowHeight*1.18f;
        drawCurtainPanel(pv.data(),curtainCenter,curtainY,curtainZ,
                         curtainPanelWidth,curtainHeight,curtainFabric);
        // Weighted hems and a narrow leading-edge welt hold the drape's tailored outline.
        drawRoundedBox(pv.data(),curtainCenter,curtainY-curtainHeight*.49f,curtainZ,
                       curtainPanelWidth,.028f,.060f,curtainFabric,0.0f,3);
        drawRoundedBox(pv.data(),curtainCenter-side*curtainPanelWidth*.49f,curtainY,curtainZ,
                       .018f,curtainHeight*.96f,.045f,curtainFabric,0.0f,3);
    }
    drawRoundedBox(pv.data(),0,curtainY+roomWindowHeight*.59f,curtainZ,
                   roomWindowWidth+curtainWidth*.9f,.065f,.12f,wood,0.0f,1);

    // A leafy floor plant and ceramic planter bring a natural material break to the back corner.
    const float plantX=-halfW+.62f, plantZ=halfD-.62f;
    const auto potColor=rgb(.12f,.14f,.22f);
    const auto leafColor=rgb(.10f,.22f,.20f);
    drawCylinder(pv.data(),plantX,floorY+.24f,plantZ,.38f,.48f,potColor,5);
    drawRoundedBox(pv.data(),plantX,floorY+.49f,plantZ,.42f,.055f,.42f,rgb(.57f,.51f,.42f),0.0f,5);
    drawSphere(pv.data(),plantX,floorY+.53f,plantZ,.16f,.035f,.16f,rgb(.16f,.12f,.08f));
    for(int stem=0;stem<5;++stem) {
        const float a=2.0f*kPi*stem/5.0f;
        const float dx=std::cos(a)*(.12f+.025f*(stem%2));
        const float dz=std::sin(a)*(.12f+.025f*(stem%2));
        drawBox(pv.data(),plantX+dx*.45f,floorY+.75f,plantZ+dz*.45f,
                .018f,.62f,.018f,rgb(.16f,.31f,.20f),a);
        drawSphere(pv.data(),plantX+dx,floorY+1.04f,plantZ+dz,.14f,.22f,.075f,leafColor);
        drawSphere(pv.data(),plantX+dx*1.35f,floorY+.87f,plantZ+dz*1.35f,.18f,.085f,.075f,leafColor);
    }
    drawSphere(pv.data(),plantX,floorY+1.34f,plantZ,.20f,.19f,.10f,leafColor);
    drawSphere(pv.data(),plantX+.12f,floorY+1.26f,plantZ-.08f,.18f,.14f,.08f,leafColor);
    drawSphere(pv.data(),plantX-.12f,floorY+1.28f,plantZ+.06f,.17f,.16f,.08f,leafColor);

    const auto accentBlue = rgb(.08f,.34f,1.0f);
    const auto accentViolet = rgb(.50f,.12f,1.0f);
    const auto neonCyan = rgb(.08f,.78f,1.0f);
    const auto neonMagenta = rgb(1.0f,.08f,.42f);
    const auto trimDark = rgb(.012f,.018f,.034f);
    const auto silver = rgb(.22f,.27f,.39f);

    // -------------------------------------------------------------------------
    // Penthouse bedroom: low upholstered platform, layered mattress, soft duvet,
    // separate pillows, tailored headboard, bedside storage and practical lamps.
    // Geometry stays inside the existing room bounds and is fully walkable around.
    // -------------------------------------------------------------------------
    const float bedX=halfW-1.21f;
    const float bedZ=backZ+1.60f;
    const auto bedBase=rgb(.018f,.024f,.043f);
    const auto bedLinen=rgb(.11f,.125f,.19f);
    const auto duvet=rgb(.043f,.052f,.086f);
    const auto pillowLinen=rgb(.20f,.22f,.30f);
    const auto bedMetal=rgb(.16f,.20f,.30f);
    drawGroundShadow(pv.data(),bedX,floorY+.004f,bedZ,3.20f,4.05f,.42f);
    // A low dark rug extends beyond the platform on all four sides.
    drawRoundedBox(pv.data(),bedX,floorY+.018f,bedZ,3.12f,.032f,3.92f,
                   rgb(.035f,.043f,.073f),0.0f,3);
    // Floating plinth, inset perimeter reveal and satin metal corner feet.
    drawRoundedBox(pv.data(),bedX,floorY+.245f,bedZ,2.42f,.34f,3.20f,bedBase,0.0f,5);
    drawRoundedBox(pv.data(),bedX,floorY+.095f,bedZ+.015f,2.30f,.035f,3.04f,
                   rgb(.035f,.046f,.078f),0.0f,5);
    drawBox(pv.data(),bedX,floorY+.105f,bedZ+1.46f,2.12f,.012f,.012f,accentViolet,0.0f,7);
    for(int side : {-1,1}) {
        drawBox(pv.data(),bedX+side*1.19f,floorY+.115f,bedZ,
                .012f,.012f,2.80f,accentBlue,0.0f,7);
        for(int end : {-1,1}) {
            drawCylinder(pv.data(),bedX+side*1.02f,floorY+.055f,bedZ+end*1.34f,
                         .075f,.095f,bedMetal,4);
        }
    }
    // Mattress foundation and a slightly crowned duvet, with stitched edge piping.
    drawPillow(pv.data(),bedX,floorY+.465f,bedZ,2.20f,.30f,2.94f,
               rgb(.18f,.20f,.27f),0.0f,0.0f,3);
    drawPillow(pv.data(),bedX,floorY+.665f,bedZ+.24f,2.06f,.18f,2.18f,
               duvet,0.0f,0.0f,3);
    drawPillow(pv.data(),bedX,floorY+.775f,bedZ+1.03f,2.02f,.09f,.66f,
               bedLinen,0.0f,0.0f,3);
    for(int side : {-1,1}) {
        drawBox(pv.data(),bedX+side*.99f,floorY+.735f,bedZ+.24f,
                .012f,.010f,1.78f,side<0?accentBlue:accentViolet,0.0f,7);
    }
    // Two full pillows and two smaller accent cushions, with real separation and overlap.
    for(int side : {-1,1}) {
        drawPillow(pv.data(),bedX+side*.49f,floorY+.835f,bedZ-.91f,
                   .91f,.17f,.60f,pillowLinen,side*.035f,0.0f,3);
        drawPillow(pv.data(),bedX+side*.28f,floorY+.905f,bedZ-.72f,
                   .47f,.15f,.43f,sofa,-side*.06f,0.0f,3);
    }
    // Tall upholstered headboard with inset panels, fine welt and a central reveal.
    const float headboardZ=bedZ-1.48f;
    drawRoundedBox(pv.data(),bedX,floorY+.955f,headboardZ,2.48f,1.32f,.17f,
                   bedBase,0.0f,5);
    drawRoundedBox(pv.data(),bedX,floorY+.98f,headboardZ+.095f,2.31f,1.13f,.035f,
                   rgb(.065f,.083f,.135f),0.0f,3);
    for(int panel=-1;panel<=1;++panel) {
        drawRoundedBox(pv.data(),bedX+panel*.72f,floorY+.99f,headboardZ+.119f,
                       .68f,1.02f,.018f,rgb(.075f,.092f,.15f),0.0f,3);
        drawBox(pv.data(),bedX+panel*.72f,floorY+.99f,headboardZ+.132f,
                .008f,.91f,.004f,bedMetal,0.0f,4);
    }
    drawBox(pv.data(),bedX,floorY+1.59f,headboardZ+.12f,2.28f,.012f,.012f,
            accentViolet,0.0f,7);
    // Floating bedside tables with drawer fronts, recessed pulls and warm-free RGB task lamps.
    for(int side : {-1,1}) {
        if(side>0) continue;
        const float nx=bedX+side*1.53f, nz=bedZ-.94f;
        drawRoundedBox(pv.data(),nx,floorY+.405f,nz,.52f,.70f,.56f,bedBase,0.0f,5);
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
                     side<0?rgb(.20f,.40f,.95f):rgb(.43f,.18f,.95f),3);
        drawCylinder(pv.data(),nx,floorY+1.035f,nz,.17f,.025f,
                     side<0?neonCyan:neonMagenta,0,0.0f);
        drawSphere(pv.data(),nx,floorY+.91f,nz,.07f,.075f,.07f,
                   side<0?accentBlue:accentViolet,7);
    }

    const float deskZ=-halfD+0.422f;
    // Soft contact shadows ground the desk, seating and chair without expensive shadow maps.
    const float shadowDeskX=-halfW+1.251f;
    const float shadowDeskZ=-halfD+0.422f;
    const float shadowDeskWidth=std::min(2.65f,halfW*1.02f);
    drawGroundShadow(pv.data(),shadowDeskX,floorY+0.004f,shadowDeskZ+0.02f,shadowDeskWidth*.82f,1.10f,.42f);
    drawGroundShadow(pv.data(),halfW-1.21f,floorY+0.004f,halfD-.62f,2.10f,.72f,.25f);
    drawGroundShadow(pv.data(),shadowDeskX-.25f,floorY+0.004f,shadowDeskZ+1.20f,.92f,1.02f,.34f);
    drawGroundShadow(pv.data(),1.12f,floorY+0.004f,.08f,1.55f,1.78f,.30f);
    drawGroundShadow(pv.data(),-halfW+.62f,floorY+0.004f,halfD-.62f,.62f,.64f,.24f);

    const float trimY = floorY + 3.72f;
    // Recessed RGB coves sit inside a dark ceiling reveal; the cores are truly emissive.
    drawRoundedBox(pv.data(),0,trimY,0,halfW*1.66f,.050f,.075f,trimDark,0.0f,5);
    drawBox(pv.data(),0,trimY-.030f,0,halfW*1.53f,.012f,.022f,accentViolet,0.0f,7);
    drawBox(pv.data(),-halfW+.12f,trimY-.08f,0,.020f,.022f,halfD*1.65f,accentBlue,0.0f,7);
    drawBox(pv.data(), halfW-.12f,trimY-.08f,0,.020f,.022f,halfD*1.65f,accentViolet,0.0f,7);
    drawBox(pv.data(),0,trimY-.08f,-halfD+.14f,halfW*1.60f,.020f,.022f,neonCyan,0.0f,7);
    // Vertical bars are recessed into the side-wall reveals, rather than floating props.
    for(int side : {-1,1}) {
        for(int bar=0;bar<3;++bar) {
            const float z=-1.8f+bar*1.65f;
            const auto tint=(bar+side+4)%2==0?neonCyan:accentViolet;
            drawBox(pv.data(),side*(halfW-.16f),floorY+2.45f,z,.055f,1.28f,.10f,trimDark,0.0f,5);
            drawBox(pv.data(),side*(halfW-.125f),floorY+2.45f,z,.010f,.91f,.026f,tint,0.0f,7);
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

    // Claude ARC gaming desk: the uploaded mesh is 2.56 m wide, 0.82 m deep,
    // and 0.75 m high. Its bounds now sit flush to the back-left room corner.
    const float deskX=-halfW+1.251f;
    const float deskWidth=2.56f;
    const float deskFrontZ=deskZ+.40f;
    const float deskTopY=floorY+.75f;
    const float deskSurface=deskTopY;
    drawGroundShadow(pv.data(),deskX,floorY+.004f,deskZ,2.72f,.98f,.48f);
    drawModel(pv.data(),deskModel_,deskX,floorY,deskZ,1.0f,0.0f,videoTexture,hasVideo);

    // Claude ARC27 curved monitor, including its own neck, hub and twin-foot base.
    // The uploaded model has a 27-inch native screen width; a modest scene scale
    // keeps its proportions while making the panel legible in the room.
    const float monitorScale=1.55f;
    const float monitorX=deskX;
    const float monitorY=deskSurface;
    const float monitorZ=deskZ-.12f;
    drawModel(pv.data(),monitorModel_,monitorX,monitorY,monitorZ,
              monitorScale,0.0f,videoTexture,hasVideo);

    // Desk mat, sculpted compact keyboard, mouse and near-field monitor speakers.
    drawRoundedBox(pv.data(),monitorX,deskSurface+.010f,deskZ+.30f,1.18f,.016f,.43f,
                   rgb(.025f,.035f,.065f),0.0f,3);
    // RGB edge-stitch on the oversized textile desk mat.
    drawBox(pv.data(),monitorX,deskSurface+.020f,deskZ+.085f,1.12f,.006f,.008f,accentBlue,0.0f,7);
    drawBox(pv.data(),monitorX,deskSurface+.020f,deskZ+.515f,1.12f,.006f,.008f,accentViolet,0.0f,7);
    drawRoundedBox(pv.data(),monitorX,deskSurface+.045f,deskZ+.30f,.83f,.052f,.205f,
                   rgb(.055f,.070f,.115f),0.0f,5);
    // Individual sculpted keycaps with blue/violet switch illumination.
    for(int row=0;row<4;++row) for(int key=0;key<12;++key) {
        const float keyX=monitorX-.34f+key*.062f;
        const float keyZ=deskZ+.225f+row*.047f;
        const bool cyanKey=((key+row)%4<2);
        drawRoundedBox(pv.data(),keyX,deskSurface+.077f,keyZ,.043f,.014f,.032f,
                       cyanKey?rgb(.34f,.43f,.58f):rgb(.28f,.24f,.52f),0.0f,5);
        drawBox(pv.data(),keyX,deskSurface+.085f,keyZ,.026f,.003f,.012f,
                cyanKey?neonCyan:accentViolet,0.0f,7);
    }
    drawRoundedBox(pv.data(),monitorX,deskSurface+.077f,deskZ+.405f,.23f,.014f,.025f,
                   rgb(.36f,.40f,.54f),0.0f,5);
    drawSphere(pv.data(),monitorX+.69f,deskSurface+.072f,deskZ+.31f,
               .095f,.045f,.145f,rgb(.16f,.19f,.23f),5);
    drawSphere(pv.data(),monitorX+.69f,deskSurface+.112f,deskZ+.285f,
               .012f,.008f,.022f,silver,4);
    // Desk-mounted broadcast mic and low-profile illuminated headphone stand.
    drawCylinder(pv.data(),monitorX-.62f,deskSurface+.26f,deskZ-.08f,.025f,.36f,silver,4,.55f);
    drawCylinder(pv.data(),monitorX-.48f,deskSurface+.42f,deskZ-.14f,.020f,.34f,silver,4,-.80f);
    drawCylinder(pv.data(),monitorX-.33f,deskSurface+.50f,deskZ-.20f,.060f,.18f,trimDark,5,.10f);
    drawSphere(pv.data(),monitorX-.33f,deskSurface+.59f,deskZ-.20f,.030f,.018f,.030f,accentBlue,7);
    drawCylinder(pv.data(),monitorX+1.02f,deskSurface+.12f,deskZ-.18f,.10f,.24f,trimDark,5);
    drawRoundedBox(pv.data(),monitorX+1.02f,deskSurface+.27f,deskZ-.18f,.065f,.12f,.055f,silver,0.0f,4);
    for(int ear : {-1,1}) {
        drawSphere(pv.data(),monitorX+1.02f+ear*.07f,deskSurface+.32f,deskZ-.18f,.055f,.075f,.040f,trimDark,5);
        drawSphere(pv.data(),monitorX+1.02f+ear*.07f,deskSurface+.32f,deskZ-.15f,.034f,.052f,.012f,accentViolet,7);
    }
    for(int side : {-1,1}) {
        const float speakerX=monitorX+side*.88f;
        drawRoundedBox(pv.data(),speakerX,deskSurface+.16f,deskZ-.15f,.145f,.29f,.16f,
                       rgb(.075f,.085f,.105f),0.0f,5);
        drawSphere(pv.data(),speakerX,deskSurface+.20f,deskZ-.065f,.046f,.046f,.012f,
                   rgb(.24f,.28f,.33f),5);
        drawSphere(pv.data(),speakerX,deskSurface+.095f,deskZ-.065f,.027f,.027f,.012f,
                   rgb(.36f,.39f,.43f),4);
    }

    // A compact white gaming PC with a glass side panel, ventilation detail and three illuminated fans.
    const float pcX=deskX+deskWidth*.31f;
    const float pcY=deskSurface+.34f;
    const float pcZ=deskZ+.12f;
    const auto pcWhite=rgb(.075f,.095f,.16f);
    const auto pcEdge=rgb(.20f,.25f,.38f);
    drawRoundedBox(pv.data(),pcX,pcY,pcZ,.43f,.66f,.57f,pcWhite,0.0f,5);
    drawBox(pv.data(),pcX,pcY,pcZ+.292f,.37f,.60f,.012f,rgb(.10f,.13f,.20f));
    drawBox(pv.data(),pcX-.16f,pcY,pcZ+.302f,.045f,.55f,.008f,rgb(.20f,.24f,.33f));
    drawBox(pv.data(),pcX+.16f,pcY,pcZ+.302f,.045f,.55f,.008f,rgb(.20f,.24f,.33f));
    for(int fan=0;fan<3;++fan) {
        const float fy=pcY+.20f-fan*.20f;
        drawSphere(pv.data(),pcX,fy,pcZ+.315f,.071f,.071f,.013f,trimDark,5);
        drawSphere(pv.data(),pcX,fy,pcZ+.329f,.060f,.060f,.010f,fan==1?accentViolet:accentBlue,7);
        drawSphere(pv.data(),pcX,fy,pcZ+.342f,.022f,.022f,.008f,rgb(.82f,.89f,1.0f),4);
        for(int blade=0;blade<4;++blade) {
            const float a=blade*kPi*.5f+fan*.18f;
            drawSphere(pv.data(),pcX+std::cos(a)*.032f,fy+std::sin(a)*.032f,pcZ+.339f,
                       .012f,.012f,.006f,rgb(.18f,.23f,.37f),5);
        }
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

    // Remaining living-area detailing stays independent of the new bed/TV wall alignment.
    // Layered living-area rug with a woven pile, stitched perimeter and inset geometric border.
    const float rugX=.40f, rugZ=.12f;
    drawRoundedBox(pv.data(),rugX,floorY+.024f,rugZ,4.00f,.045f,3.72f,
                   rgb(.17f,.20f,.24f),0.0f,3);
    drawRoundedBox(pv.data(),rugX,floorY+.048f,rugZ,3.80f,.009f,3.52f,
                   rgb(.25f,.27f,.29f),0.0f,3);
    for(int line=-1;line<=1;++line) {
        drawBox(pv.data(),rugX+line*.19f,floorY+.054f,rugZ, .012f,.004f,2.98f,
                rgb(.36f,.34f,.29f),0.0f,3);
        drawBox(pv.data(),rugX,floorY+.054f,rugZ+line*.19f,3.24f,.004f,.012f,
                rgb(.36f,.34f,.29f),0.0f,3);
    }

    // Low coffee table with a softly bevelled walnut top, recessed shelf and satin legs.
    const float coffeeX=1.10f, coffeeZ=.08f, coffeeY=floorY+.39f;
    drawRoundedBox(pv.data(),coffeeX,coffeeY,coffeeZ,1.20f,.075f,1.40f,wood,0.0f,1);
    drawRoundedBox(pv.data(),coffeeX,coffeeY-.15f,coffeeZ,1.00f,.045f,1.18f,
                   rgb(.23f,.16f,.11f),0.0f,1);
    for(int side : {-1,1}) for(int end : {-1,1}) {
        drawCylinder(pv.data(),coffeeX+side*.48f,coffeeY-.22f,coffeeZ+end*.57f,
                     .055f,.36f,silver,4);
    }
    // A pair of stacked hardback books and a small ceramic cup on a coaster.
    drawRoundedBox(pv.data(),coffeeX-.20f,coffeeY+.052f,coffeeZ-.20f,.39f,.045f,.29f,
                   rgb(.20f,.25f,.31f),-.08f,5);
    drawRoundedBox(pv.data(),coffeeX-.17f,coffeeY+.105f,coffeeZ-.19f,.34f,.045f,.27f,
                   rgb(.49f,.26f,.18f),.06f,5);
    drawBox(pv.data(),coffeeX-.17f,coffeeY+.107f,coffeeZ-.19f,.29f,.003f,.23f,
            rgb(.80f,.76f,.66f),.06f);
    drawCylinder(pv.data(),coffeeX+.30f,coffeeY+.085f,coffeeZ+.27f,
                  .15f,.17f,rgb(.76f,.72f,.64f),5);
    drawSphere(pv.data(),coffeeX+.30f,coffeeY+.172f,coffeeZ+.27f,.060f,.014f,.060f,
               rgb(.14f,.09f,.055f));
    drawSphere(pv.data(),coffeeX+.38f,coffeeY+.08f,coffeeZ+.27f,.040f,.045f,.015f,
               rgb(.76f,.72f,.64f),5);
    // A compact controller rests beside the books on the desk.
    drawRoundedBox(pv.data(),deskX-.90f,deskSurface+.035f,deskZ+.25f,.23f,.055f,.15f,
                   rgb(.12f,.15f,.20f),0.0f,5);
    drawSphere(pv.data(),deskX-.99f,deskSurface+.058f,deskZ+.25f,.055f,.035f,.060f,
               rgb(.12f,.15f,.20f),5);
    drawSphere(pv.data(),deskX-.81f,deskSurface+.058f,deskZ+.25f,.055f,.035f,.060f,
               rgb(.12f,.15f,.20f),5);
    drawSphere(pv.data(),deskX-.90f,deskSurface+.068f,deskZ+.25f,.018f,.012f,.018f,accentBlue);


    if(settings_.showTv) {
        // Wall-mounted television is on the front wall, directly opposite and centered
        // on the corner bed. Its back is flush to the wall and its screen faces the bed.
        const float tvX=0.0f, tvY=floorY+2.04f, tvZ=frontZ-.12f;
        const float tvW=std::min(2.35f,halfW*.62f), tvH=tvW*9.0f/16.0f;
        const bool tvStream=settings_.screenDestination=="tv"||settings_.screenDestination=="both";
        const GLuint tvTex=(tvStream&&hasVideo)?videoTexture:env;
        drawGroundShadow(pv.data(),tvX,floorY+.004f,tvZ-.38f,2.10f,.72f,.25f);
        drawRoundedBox(pv.data(),tvX,tvY,tvZ,tvW+.18f,tvH+.18f,.12f,trimDark,0.0f,5);
        drawRoundedBox(pv.data(),tvX,tvY,tvZ-.064f,tvW+.045f,tvH+.045f,.012f,silver,0.0f,4);
        drawPlane(pv.data(),tvX,tvY,tvZ-.073f,tvW,tvH,tvTex,rgb(1,1,1),kPi);
        drawBox(pv.data(),tvX,tvY-tvH*.5f-.075f,tvZ-.080f,tvW*.34f,.010f,.006f,accentBlue,0.0f,7);

        // Low floating media console follows the same front-wall alignment as the TV.
        const float consoleY=floorY+.42f, consoleZ=tvZ-.34f, consoleW=2.05f;
        drawRoundedBox(pv.data(),tvX,consoleY,consoleZ,consoleW,.54f,.48f,trimDark,0.0f,5);
        drawRoundedBox(pv.data(),tvX,consoleY+.255f,consoleZ,consoleW+.04f,.045f,.51f,silver,0.0f,4);
        for(int bay=0;bay<3;++bay) {
            const float panelX=tvX+(bay-1)*.66f;
            drawRoundedBox(pv.data(),panelX,consoleY+.015f,consoleZ-.245f,.59f,.37f,.018f,
                           rgb(.045f,.060f,.095f),0.0f,5);
            drawBox(pv.data(),panelX,consoleY+.06f,consoleZ-.258f,.10f,.008f,.006f,
                    bay==1?accentViolet:accentBlue,0.0f,7);
        }
        drawBox(pv.data(),tvX, floorY+.135f,consoleZ-.10f,consoleW*.88f,.012f,.012f,accentBlue,0.0f,7);
        // Compact vertical console and controller dock rest on the cabinet's right end.
        drawRoundedBox(pv.data(),tvX+consoleW*.40f,floorY+.82f,consoleZ-.04f,
                       .18f,.70f,.31f,rgb(.74f,.78f,.86f),0.0f,5);
        drawBox(pv.data(),tvX+consoleW*.40f,floorY+.82f,consoleZ-.205f,
                .008f,.58f,.008f,rgb(.20f,.24f,.32f));
        drawSphere(pv.data(),tvX+consoleW*.40f,floorY+.55f,consoleZ-.215f,
                   .010f,.010f,.010f,accentBlue,7);
    }

    // Tailored sofa: a rigid upholstered frame carries separate domed seat and back cushions.
    // The cushion mesh has sewn gussets and a rounded crown rather than ellipsoidal placeholder blobs.
    if(settings_.showCouch) {
        const float couchX=halfW-2.15f, couchZ=.55f, couchYaw=kPi*.5f;
        const float cx=couchX;
        // Sofa frame axes follow the room: width across X, full seat length along Z.
        drawRoundedBox(pv.data(),cx,floorY+.38f,couchZ,.88f,.25f,1.92f,trimDark,0.0f,5);
        // Back shell is on -X, directly behind the two cushions that face the TV at +X.
        drawRoundedBox(pv.data(),cx-.20f,floorY+.66f,couchZ,.24f,.47f,1.82f,trimDark,0.0f,5);
        // Separate seat cushions sit proud of the upholstered deck with a visible center split.
        for(int seat=-1;seat<=1;seat+=2) {
            drawPillow(pv.data(),cx+.10f,floorY+.535f,couchZ+seat*.34f,
                       .70f,.13f,.67f,sofa,couchYaw,0.0f,3);
        }
        // The back pads face the television and lean gently into the sofa frame.
        for(int pad=-1;pad<=1;pad+=2) {
            drawPillow(pv.data(),cx-.31f,floorY+.78f,couchZ+pad*.39f,
                       .72f,.16f,.62f,sofa,couchYaw,kPi*.5f,3);
        }
        // Soft, broad arm caps and welt strips finish both exposed ends.
        for(int end : {-1,1}) {
            drawPillow(pv.data(),cx+.02f,floorY+.625f,couchZ+end*.86f,
                       .40f,.17f,.27f,sofa,couchYaw,0.0f,3);
            drawBox(pv.data(),cx-.10f,floorY+.62f,couchZ+end*.86f,
                    .010f,.008f,.28f,wallHighlight);
        }
        // Four recessed legs are set in from the corners.
        for(int side : {-1,1}) for(int end : {-1,1}) {
            const float legX=cx+side*.30f,legZ=couchZ+end*.70f;
            drawCylinder(pv.data(),legX,floorY+.13f,legZ,.075f,.24f,trimDark,4);
            drawCylinder(pv.data(),legX,floorY+.035f,legZ,.082f,.035f,silver,4);
        }
    }

    // Ergonomic task chair with a molded back shell, domed seat and independent lumbar pad.
    if(settings_.showChair) {
        const float chairX=deskX+.25f, chairZ=deskZ+1.20f;
        drawRoundedBox(pv.data(),chairX,floorY+.62f,chairZ,.70f,.17f,.68f,trimDark,0.0f,5);
        drawPillow(pv.data(),chairX,floorY+.665f,chairZ,.62f,.15f,.58f,sofa,0.0f,0.0f,3);
        // A tapered support shell sits behind the fabric backrest.
        drawRoundedBox(pv.data(),chairX,floorY+1.03f,chairZ-.23f,.61f,.83f,.15f,trimDark,0.0f,5);
        drawPillow(pv.data(),chairX,floorY+1.03f,chairZ-.135f,.55f,.14f,.72f,sofa,0.0f,kPi*.5f,3);
        drawPillow(pv.data(),chairX,floorY+.94f,chairZ-.045f,.43f,.10f,.20f,
                   rgb(sofa[0]*1.12f,sofa[1]*1.12f,sofa[2]*1.12f),0.0f,kPi*.5f,3);
        // Padded armrests sit on swept metal supports instead of bare spherical stubs.
        for(int side : {-1,1}) {
            drawRoundedBox(pv.data(),chairX+side*.31f,floorY+.56f,chairZ-.04f,
                           .045f,.30f,.055f,silver,side*.11f,4);
            drawPillow(pv.data(),chairX+side*.32f,floorY+.72f,chairZ+.01f,
                       .15f,.085f,.37f,sofa,0.0f,0.0f,3);
        }
        // Height-adjustable gas lift, five-spoke metal base, and individual caster wheels.
        drawCylinder(pv.data(),chairX,floorY+.28f,chairZ,.075f,.42f,silver,4);
        drawCylinder(pv.data(),chairX,floorY+.075f,chairZ,.12f,.10f,trimDark,5);
        for(int i=0;i<5;++i) {
            const float a=2*kPi*i/5;
            const float wx=chairX+std::cos(a)*.36f,wz=chairZ+std::sin(a)*.32f;
            drawRoundedBox(pv.data(),chairX+std::cos(a)*.20f,floorY+.075f,
                           chairZ+std::sin(a)*.18f,.38f,.055f,.075f,silver,a,4);
            drawSphere(pv.data(),wx,floorY+.045f,wz,.075f,.075f,.052f,trimDark,5);
            drawCylinder(pv.data(),wx,floorY+.045f,wz,.035f,.09f,silver,4);
        }
    }

    // A finished floor lamp: weighted base, slender stem, turned collar, tapered textile shade and diffuser.
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
    // Monitor-anchored menu: world coordinates place it on the virtual display, not in the user's face.
    if (menuVisible_) {
        const float deskZ = -halfD + 0.422f;
        const float deskX = -halfW + 1.251f;
        const float monitorX = deskX;
        const float monitorY = floorY + 0.75f;
        const float monitorZ = deskZ - 0.12f;
        const float menuZ = monitorZ + 0.075f;
        const auto panel = rgb(0.025f, 0.032f, 0.065f, 0.98f);
        const auto row = rgb(0.075f, 0.090f, 0.145f, 0.98f);
        const auto selected = rgb(0.10f, 0.22f, 0.39f, 1.0f);
        const auto blue = rgb(0.16f, 0.58f, 1.0f, 1.0f);
        const auto violet = rgb(0.48f, 0.25f, 1.0f, 1.0f);
        const auto text = rgb(0.88f, 0.92f, 1.0f, 1.0f);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(program_);
        drawBox(pv.data(), monitorX, monitorY, menuZ, 0.76f, 0.42f, 0.008f, panel);
        drawBox(pv.data(), monitorX, monitorY + 0.190f, menuZ + 0.006f, 0.70f, 0.010f, 0.006f, blue);
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
    if(roundedVao_)glDeleteVertexArrays(1,&roundedVao_);if(roundedVbo_)glDeleteBuffers(1,&roundedVbo_);
    if(pillowVao_)glDeleteVertexArrays(1,&pillowVao_);if(pillowVbo_)glDeleteBuffers(1,&pillowVbo_);
    if(curtainVao_)glDeleteVertexArrays(1,&curtainVao_);if(curtainVbo_)glDeleteBuffers(1,&curtainVbo_);
    if(cylinderVao_)glDeleteVertexArrays(1,&cylinderVao_);if(cylinderVbo_)glDeleteBuffers(1,&cylinderVbo_);
    for(auto t:environmentTextures_)if(t)glDeleteTextures(1,&t);if(videoFallbackTexture_)glDeleteTextures(1,&videoFallbackTexture_);
    if(groundShadowTexture_)glDeleteTextures(1,&groundShadowTexture_);
    if(materialAtlasTexture_)glDeleteTextures(1,&materialAtlasTexture_);
    if(normalAtlasTexture_)glDeleteTextures(1,&normalAtlasTexture_);
    if(roughnessAtlasTexture_)glDeleteTextures(1,&roughnessAtlasTexture_);
    if(deskSurfaceTexture_)glDeleteTextures(1,&deskSurfaceTexture_);
    for(auto& mesh:deskModel_) {
        if(mesh.vao)glDeleteVertexArrays(1,&mesh.vao);
        if(mesh.vbo)glDeleteBuffers(1,&mesh.vbo);
        if(mesh.ebo)glDeleteBuffers(1,&mesh.ebo);
    }
    for(auto& mesh:monitorModel_) {
        if(mesh.vao)glDeleteVertexArrays(1,&mesh.vao);
        if(mesh.vbo)glDeleteBuffers(1,&mesh.vbo);
        if(mesh.ebo)glDeleteBuffers(1,&mesh.ebo);
    }
    deskModel_.clear();
    monitorModel_.clear();
    program_=vignetteProgram_=vao_=vbo_=planeTexture_=cubeTexture_=videoFallbackTexture_=groundShadowTexture_=materialAtlasTexture_=normalAtlasTexture_=roughnessAtlasTexture_=deskSurfaceTexture_=sphereVao_=sphereVbo_=textVao_=textVbo_=curvedVao_=curvedVbo_=0;roundedVao_=roundedVbo_=pillowVao_=pillowVbo_=curtainVao_=curtainVbo_=cylinderVao_=cylinderVbo_=0;sphereVertexCount_=roundedVertexCount_=pillowVertexCount_=curtainVertexCount_=cylinderVertexCount_=0;environmentTextures_.fill(0);initialized_=false;
}
