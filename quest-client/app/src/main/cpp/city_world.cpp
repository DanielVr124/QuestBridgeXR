#include "city_world.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iterator>
#include <vector>

namespace {
constexpr float kChunkSize = 84.0f;
constexpr float kPi = 3.14159265358979323846f;

const char* kCityVertexShader = R"GLSL(#version 300 es
precision highp float;
precision highp int;
layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUv;
layout(location=3) in vec3 aColor;
layout(location=4) in float aMaterial;
uniform mat4 uViewProjection;
out vec3 vWorld;
out vec3 vNormal;
out vec2 vUv;
out vec3 vColor;
flat out int vMaterial;
void main() {
    vWorld = aPosition;
    vNormal = aNormal;
    vUv = aUv;
    vColor = aColor;
    vMaterial = int(aMaterial + 0.5);
    gl_Position = uViewProjection * vec4(aPosition, 1.0);
}
)GLSL";

const char* kCityFragmentShader = R"GLSL(#version 300 es
precision highp float;
precision highp int;
in vec3 vWorld;
in vec3 vNormal;
in vec2 vUv;
in vec3 vColor;
flat in int vMaterial;
uniform vec3 uCameraPosition;
uniform vec3 uSunDirection;
out vec4 outColor;

float hash21(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}
float noise2(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash21(i), hash21(i + vec2(1.0,0.0)), f.x),
               mix(hash21(i + vec2(0.0,1.0)), hash21(i + vec2(1.0,1.0)), f.x), f.y);
}
void main() {
    vec3 n = normalize(vNormal);
    vec3 viewDir = normalize(uCameraPosition - vWorld);
    vec3 sunDir = normalize(uSunDirection);
    vec3 albedo = vColor;
    float roughness = 0.82;
    float metallic = 0.02;
    float surfaceNoise = noise2(vWorld.xz * 0.73 + vUv * 0.23);

    if (vMaterial == 0) {                 // poured concrete / stone
        albedo *= 0.91 + surfaceNoise * 0.13;
        albedo += (noise2(vWorld.xz * 3.7) - 0.5) * 0.025;
        roughness = 0.91;
    } else if (vMaterial == 1) {          // asphalt
        float grain = noise2(vWorld.xz * 8.0) * 0.08 + noise2(vWorld.xz * 29.0) * 0.035;
        albedo *= 0.77 + grain;
        roughness = 0.96;
    } else if (vMaterial == 2) {          // architectural glass
        float mullionNoise = noise2(vWorld.xy * 2.3);
        albedo *= 0.75 + mullionNoise * 0.12;
        metallic = 0.48;
        roughness = 0.19;
    } else if (vMaterial == 3) {          // brick / masonry
        vec2 brickUv = vUv * vec2(2.3, 1.0);
        vec2 cell = fract(brickUv + vec2(0.5 * mod(floor(brickUv.y), 2.0), 0.0));
        float mortar = step(0.035, cell.x) * step(0.045, cell.y) *
                       (1.0 - step(0.965, cell.x)) * (1.0 - step(0.955, cell.y));
        albedo *= mix(0.62, 1.04, mortar) * (0.92 + surfaceNoise * 0.13);
        roughness = 0.93;
    } else if (vMaterial == 4) {          // painted / brushed metal
        albedo *= 0.92 + noise2(vWorld.xy * 11.0) * 0.10;
        metallic = 0.62;
        roughness = 0.34;
    } else if (vMaterial == 5) {          // foliage
        albedo *= 0.78 + noise2(vWorld.xz * 4.0) * 0.32;
        roughness = 0.95;
    } else if (vMaterial == 6) {          // road paint
        albedo *= 0.92 + noise2(vWorld.xz * 23.0) * 0.06;
        roughness = 0.72;
    } else if (vMaterial == 7) {          // emissive practicals / lit windows
        outColor = vec4(albedo * 1.32, 1.0);
        return;
    } else if (vMaterial == 8) {          // water / polished stone
        albedo *= 0.88 + surfaceNoise * 0.08;
        metallic = 0.28;
        roughness = 0.24;
    }

    float ndl = max(dot(n, sunDir), 0.0);
    vec3 skyFill = mix(vec3(0.20,0.24,0.30), vec3(0.58,0.66,0.76), max(n.y,0.0));
    vec3 warmSun = vec3(1.0,0.86,0.70) * (0.28 + ndl * 1.02);
    vec3 ambient = skyFill * (0.48 + max(n.y,0.0)*0.16);
    vec3 halfDir = normalize(sunDir + viewDir);
    float specPower = mix(10.0, 100.0, 1.0 - roughness);
    float specular = pow(max(dot(n, halfDir),0.0), specPower) *
                     (0.035 + metallic * 0.24) * (0.35 + ndl);
    float fresnel = pow(1.0 - max(dot(n,viewDir),0.0), 5.0);
    if (vMaterial == 2) {
        specular += fresnel * 0.20;
        albedo = mix(albedo, vec3(0.48,0.63,0.73), fresnel * 0.26);
    }
    vec3 lit = albedo * (ambient + warmSun * 0.74) + vec3(specular);
    float distanceToCamera = length(uCameraPosition - vWorld);
    float fog = smoothstep(135.0, 315.0, distanceToCamera);
    vec3 horizon = vec3(0.62,0.69,0.75);
    lit = mix(lit, horizon, fog);
    outColor = vec4(max(lit, vec3(0.0)), 1.0);
}
)GLSL";

const char* kSkyVertexShader = R"GLSL(#version 300 es
precision highp float;
layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aColor;
uniform mat4 uViewProjection;
out vec3 vColor;
out float vHeight;
void main() {
    vColor = aColor;
    vHeight = aPosition.y;
    gl_Position = uViewProjection * vec4(aPosition, 1.0);
}
)GLSL";

const char* kSkyFragmentShader = R"GLSL(#version 300 es
precision highp float;
in vec3 vColor;
in float vHeight;
out vec4 outColor;
void main() {
    vec3 haze = vec3(0.67,0.74,0.80);
    float horizon = smoothstep(-0.12,0.72,vHeight);
    vec3 sky = mix(haze, vec3(0.24,0.43,0.64), horizon);
    float warm = (1.0 - smoothstep(-0.05,0.25,vHeight)) * 0.20;
    sky = mix(sky, vec3(0.92,0.68,0.48), warm);
    outColor = vec4(mix(sky, vColor, 0.12), 1.0);
}
)GLSL";

GLuint compile(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok != GL_TRUE) {
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint linkProgram(const char* vertexSource, const char* fragmentSource) {
    GLuint vs = compile(GL_VERTEX_SHADER, vertexSource);
    GLuint fs = compile(GL_FRAGMENT_SHADER, fragmentSource);
    if (!vs || !fs) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return 0;
    }
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);
    GLint ok = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (ok != GL_TRUE) {
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

float clampf(float value, float lo, float hi) {
    return std::max(lo, std::min(value, hi));
}

} // namespace

std::int64_t CityWorld::chunkKey(int x, int z) {
    const std::uint64_t packed=(static_cast<std::uint64_t>(static_cast<std::uint32_t>(x))<<32) |
                              static_cast<std::uint32_t>(z);
    return static_cast<std::int64_t>(packed);
}

std::uint32_t CityWorld::hash(std::uint32_t value) {
    value ^= value >> 16;
    value *= 0x7feb352dU;
    value ^= value >> 15;
    value *= 0x846ca68bU;
    value ^= value >> 16;
    return value;
}

float CityWorld::random01(std::uint32_t seed) {
    return static_cast<float>(hash(seed) & 0x00ffffffU) / 16777215.0f;
}

bool CityWorld::initialize() {
    if (initialized_) return true;
    program_ = linkProgram(kCityVertexShader, kCityFragmentShader);
    skyProgram_ = linkProgram(kSkyVertexShader, kSkyFragmentShader);
    if (!program_ || !skyProgram_) {
        release();
        return false;
    }
    mvpLocation_ = glGetUniformLocation(program_, "uViewProjection");
    cameraLocation_ = glGetUniformLocation(program_, "uCameraPosition");
    sunLocation_ = glGetUniformLocation(program_, "uSunDirection");

    // A compact 32-triangle atmospheric dome. It is rebuilt around the tracked camera,
    // so the horizon stays fixed while walking without using costly real-time sky lights.
    std::vector<float> sky;
    constexpr int rings = 9;
    constexpr int sectors = 32;
    for (int ring = 0; ring < rings; ++ring) {
        const float a0 = (static_cast<float>(ring) / rings) * (kPi * 0.49f);
        const float a1 = (static_cast<float>(ring + 1) / rings) * (kPi * 0.49f);
        for (int sector = 0; sector < sectors; ++sector) {
            const float t0 = (static_cast<float>(sector) / sectors) * 2.0f * kPi;
            const float t1 = (static_cast<float>(sector + 1) / sectors) * 2.0f * kPi;
            const auto point = [](float a, float t) {
                const float c = std::cos(a);
                return std::array<float,3>{c * std::cos(t), std::sin(a), c * std::sin(t)};
            };
            const auto p0 = point(a0,t0), p1 = point(a0,t1);
            const auto p2 = point(a1,t1), p3 = point(a1,t0);
            const std::array<float,3> colors[4] = {
                {0.78f,0.72f,0.64f},{0.77f,0.73f,0.67f},
                {0.60f,0.70f,0.82f},{0.60f,0.70f,0.82f}
            };
            const std::array<std::array<float,3>,6> tri = {p0,p1,p2,p0,p2,p3};
            const std::array<int,6> colorIndex = {0,1,2,0,2,3};
            for (int i=0;i<6;++i) {
                sky.insert(sky.end(), {tri[i][0],tri[i][1],tri[i][2],
                    colors[colorIndex[i]][0],colors[colorIndex[i]][1],colors[colorIndex[i]][2]});
            }
        }
    }
    skyVertexCount_ = static_cast<GLsizei>(sky.size()/6u);
    glGenVertexArrays(1,&skyVao_);
    glGenBuffers(1,&skyVbo_);
    glBindVertexArray(skyVao_);
    glBindBuffer(GL_ARRAY_BUFFER,skyVbo_);
    glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(sky.size()*sizeof(float)),sky.data(),GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),reinterpret_cast<void*>(3*sizeof(float)));
    glBindVertexArray(0);
    initialized_ = true;
    return true;
}

void CityWorld::release() {
    for (auto& entry : chunks_) {
        if (entry.second.vao) glDeleteVertexArrays(1,&entry.second.vao);
        if (entry.second.vbo) glDeleteBuffers(1,&entry.second.vbo);
    }
    chunks_.clear();
    if (skyVao_) glDeleteVertexArrays(1,&skyVao_);
    if (skyVbo_) glDeleteBuffers(1,&skyVbo_);
    if (program_) glDeleteProgram(program_);
    if (skyProgram_) glDeleteProgram(skyProgram_);
    skyVao_=skyVbo_=program_=skyProgram_=0;
    skyVertexCount_=0;
    initialized_=false;
}

void CityWorld::addQuad(std::vector<Vertex>& out, const float* a, const float* b,
                        const float* c, const float* d, const float* color,
                        float material, float repeatU, float repeatV) {
    const float ux=b[0]-a[0], uy=b[1]-a[1], uz=b[2]-a[2];
    const float vx=d[0]-a[0], vy=d[1]-a[1], vz=d[2]-a[2];
    float nx=uy*vz-uz*vy, ny=uz*vx-ux*vz, nz=ux*vy-uy*vx;
    const float length=std::max(std::sqrt(nx*nx+ny*ny+nz*nz),0.00001f);
    nx/=length; ny/=length; nz/=length;
    const Vertex vertices[6] = {
        {a[0],a[1],a[2],nx,ny,nz,0,0,color[0],color[1],color[2],material},
        {b[0],b[1],b[2],nx,ny,nz,repeatU,0,color[0],color[1],color[2],material},
        {c[0],c[1],c[2],nx,ny,nz,repeatU,repeatV,color[0],color[1],color[2],material},
        {a[0],a[1],a[2],nx,ny,nz,0,0,color[0],color[1],color[2],material},
        {c[0],c[1],c[2],nx,ny,nz,repeatU,repeatV,color[0],color[1],color[2],material},
        {d[0],d[1],d[2],nx,ny,nz,0,repeatV,color[0],color[1],color[2],material}
    };
    out.insert(out.end(),std::begin(vertices),std::end(vertices));
}

void CityWorld::addBox(std::vector<Vertex>& out, float x, float y, float z,
                       float sx, float sy, float sz, const float* color,
                       float material, float yaw) {
    const float hx=sx*.5f, hy=sy*.5f, hz=sz*.5f;
    const float c=std::cos(yaw), s=std::sin(yaw);
    auto point=[&](float px,float py,float pz) {
        return std::array<float,3>{x+c*px+s*pz,y+py,z-s*px+c*pz};
    };
    auto face=[&](std::array<float,3> a,std::array<float,3> b,
                  std::array<float,3> cc,std::array<float,3> d) {
        addQuad(out,a.data(),b.data(),cc.data(),d.data(),color,material,
                std::max(1.0f,sx*.35f),std::max(1.0f,sy*.35f));
    };
    face(point(-hx,-hy,hz),point(hx,-hy,hz),point(hx,hy,hz),point(-hx,hy,hz));
    face(point(hx,-hy,-hz),point(-hx,-hy,-hz),point(-hx,hy,-hz),point(hx,hy,-hz));
    face(point(hx,-hy,hz),point(hx,-hy,-hz),point(hx,hy,-hz),point(hx,hy,hz));
    face(point(-hx,-hy,-hz),point(-hx,-hy,hz),point(-hx,hy,hz),point(-hx,hy,-hz));
    face(point(-hx,hy,hz),point(hx,hy,hz),point(hx,hy,-hz),point(-hx,hy,-hz));
    face(point(-hx,-hy,-hz),point(hx,-hy,-hz),point(hx,-hy,hz),point(-hx,-hy,hz));
}

void CityWorld::addOctahedron(std::vector<Vertex>& out, float x, float y, float z,
                              float rx, float ry, float rz, const float* color) {
    const std::array<float,3> top={x,y+ry,z};
    const std::array<float,3> bottom={x,y-ry,z};
    const std::array<float,3> east={x+rx,y,z};
    const std::array<float,3> north={x,y,z+rz};
    const std::array<float,3> west={x-rx,y,z};
    const std::array<float,3> south={x,y,z-rz};
    auto triangle=[&](std::array<float,3> a,std::array<float,3> b,std::array<float,3> c) {
        const float ux=b[0]-a[0],uy=b[1]-a[1],uz=b[2]-a[2];
        const float vx=c[0]-a[0],vy=c[1]-a[1],vz=c[2]-a[2];
        const float nx=uy*vz-uz*vy,ny=uz*vx-ux*vz,nz=ux*vy-uy*vx;
        const float mx=(a[0]+b[0]+c[0])/3.0f-x;
        const float my=(a[1]+b[1]+c[1])/3.0f-y;
        const float mz=(a[2]+b[2]+c[2])/3.0f-z;
        if(nx*mx+ny*my+nz*mz<0.0f) std::swap(b,c);
        addQuad(out,a.data(),b.data(),c.data(),c.data(),color,5.0f);
    };
    triangle(top,north,east); triangle(top,west,north);
    triangle(top,south,west); triangle(top,east,south);
    triangle(bottom,east,north); triangle(bottom,north,west);
    triangle(bottom,west,south); triangle(bottom,south,east);
}

void CityWorld::addTree(std::vector<Vertex>& out, float x, float z, float scale, std::uint32_t seed) {
    const float trunk[3]={0.28f,0.20f,0.13f};
    const float leafA[3]={0.16f,0.29f,0.19f};
    const float leafB[3]={0.21f,0.34f,0.22f};
    const float leafC[3]={0.28f,0.38f,0.25f};
    addBox(out,x,1.02f*scale,z,.25f*scale,2.04f*scale,.25f*scale,trunk,3);
    const float jitterX=(random01(seed+1u)-0.5f)*.18f*scale;
    const float jitterZ=(random01(seed+2u)-0.5f)*.18f*scale;
    addOctahedron(out,x+jitterX,2.30f*scale,z+jitterZ,.70f*scale,.87f*scale,.66f*scale,leafA);
    addOctahedron(out,x-.33f*scale,2.02f*scale,z+.14f*scale,.55f*scale,.64f*scale,.58f*scale,leafB);
    addOctahedron(out,x+.32f*scale,2.10f*scale,z-.20f*scale,.58f*scale,.66f*scale,.57f*scale,leafC);
    addOctahedron(out,x,2.88f*scale,z,.42f*scale,.55f*scale,.40f*scale,leafB);
}

void CityWorld::addBuilding(std::vector<Vertex>& out, float x, float z, float width,
                            float depth, float height, int style, std::uint32_t seed, bool detailed) {
    const float baseY=0.16f;
    const float stoneA[3]={0.60f,0.59f,0.55f};
    const float stoneB[3]={0.43f,0.48f,0.51f};
    const float brick[3]={0.49f,0.28f,0.20f};
    const float precast[3]={0.48f,0.51f,0.52f};
    const float glass[3]={0.12f,0.22f,0.31f};
    const float facade[3] = {
        style==0?brick[0]:(style==1?stoneA[0]:(style==2?glass[0]:(style==3?precast[0]:stoneB[0]))),
        style==0?brick[1]:(style==1?stoneA[1]:(style==2?glass[1]:(style==3?precast[1]:stoneB[1]))),
        style==0?brick[2]:(style==1?stoneA[2]:(style==2?glass[2]:(style==3?precast[2]:stoneB[2])))
    };
    const float roofColor[3]={0.20f,0.23f,0.25f};
    addBox(out,x,height*.5f+baseY,z,width,height,depth,facade,style==0?3.0f:0.0f);

    const float frame[3]={0.72f,0.73f,0.72f};
    const float sill[3]={0.33f,0.36f,0.37f};
    const float warmWindow[3]={0.95f,0.67f,0.38f};
    const float coolGlass[3]={0.075f,0.17f,0.24f};
    const int floors=std::clamp(static_cast<int>((height-1.0f)/3.25f),3,12);
    const int columns=std::clamp(static_cast<int>((width-1.2f)/2.7f),2,4);
    const float floorHeight=(height-0.65f)/floors;
    if (detailed) {
        for (int side : {-1,1}) {
            const float zFace=z+side*(depth*.5f+.027f);
            for (int floor=0;floor<floors;++floor) {
                const float y=baseY+0.75f+floor*floorHeight;
                for (int col=0;col<columns;++col) {
                    const float wx=x-width*.5f+0.72f+(col+.5f)*(width-1.44f)/columns;
                    const float ww=std::min(2.05f,(width-1.5f)/columns*.74f);
                    const float wh=std::min(1.92f,floorHeight*.68f);
                    const bool lit=(hash(seed+static_cast<std::uint32_t>(floor*97+col*13+side*3))%100u)<13u;
                    const float* pane=lit?warmWindow:coolGlass;
                    addBox(out,wx,y+0.18f,zFace,ww,wh,.035f,pane,lit?7.0f:2.0f);
                    addBox(out,wx,y-wh*.5f+0.18f,zFace+.028f,ww+.06f,.055f,.07f,sill,4);
                    addBox(out,wx,y+wh*.5f+0.18f,zFace+.028f,ww+.06f,.045f,.07f,frame,4);
                }
                addBox(out,x,y+floorHeight-.12f,zFace, width*.95f,.075f,.075f,
                       style==0?brick:frame,style==0?3.0f:4.0f);
            }
        }
        for (int side : {-1,1}) {
            const float xFace=x+side*(width*.5f+.027f);
            const int sideColumns=std::clamp(static_cast<int>((depth-1.0f)/3.1f),2,4);
            for (int floor=0;floor<floors;++floor) {
                const float y=baseY+0.75f+floor*floorHeight;
                for (int col=0;col<sideColumns;++col) {
                    const float wz=z-depth*.5f+.62f+(col+.5f)*(depth-1.24f)/sideColumns;
                    const float ww=std::min(2.00f,(depth-1.3f)/sideColumns*.72f);
                    const float wh=std::min(1.84f,floorHeight*.68f);
                    const bool lit=(hash(seed+static_cast<std::uint32_t>(floor*67+col*19+side*5))%100u)<11u;
                    const float* pane=lit?warmWindow:coolGlass;
                    addBox(out,xFace,y+0.18f,wz,.035f,wh,ww,pane,lit?7.0f:2.0f);
                    addBox(out,xFace+.028f,y-wh*.5f+0.18f,wz,.07f,.05f,ww+.06f,sill,4);
                    addBox(out,xFace+.028f,y+wh*.5f+0.18f,wz,.07f,.04f,ww+.06f,frame,4);
                }
            }
        }
    } else {
        // Distance LOD: broad glazing bands and fewer facade divisions, retaining the silhouette.
        const float bandColor[3]={0.10f,0.20f,0.28f};
        for (int floor=1;floor<std::max(2,floors);floor+=2) {
            const float y=baseY+floor*floorHeight;
            addBox(out,x,y,z+depth*.5f+.035f,width*.82f,1.15f,.045f,bandColor,2);
            addBox(out,x,y,z-depth*.5f-.035f,width*.82f,1.15f,.045f,bandColor,2);
        }
    }

    // A clearly articulated street-level shopfront, recessed entrance and canopy.
    const float shopGlass[3]={0.09f,0.18f,0.23f};
    if (height>9.0f) {
        addBox(out,x,1.75f,z+depth*.5f+.06f,width*.76f,2.55f,.08f,shopGlass,2);
        addBox(out,x,3.22f,z+depth*.5f+.16f,width*.91f,.18f,.25f,
               style==0?brick:frame,4);
        addBox(out,x-width*.30f,1.40f,z+depth*.5f+.13f,.12f,2.05f,.12f,frame,4);
        addBox(out,x+width*.30f,1.40f,z+depth*.5f+.13f,.12f,2.05f,.12f,frame,4);
    }

    // Parapet, roof equipment and a small setback mechanical penthouse.
    addBox(out,x,height+baseY+.16f,z,width+.20f,.32f,depth+.20f,roofColor,4);
    const float hvac[3]={0.40f,0.43f,0.43f};
    for (int unit=0;unit<(detailed?2:1);++unit) {
        const float offset=(unit==0?-1.0f:1.0f)*width*.22f;
        addBox(out,x+offset,height+baseY+.64f,z,.95f,.72f,.80f,hvac,4);
        addBox(out,x+offset,height+baseY+1.02f,z,.65f,.06f,.56f,frame,4);
    }
    // Slightly different parapet/corner features break up repeated modular towers.
    if ((hash(seed+0x331u)%3u)==0u) {
        addBox(out,x+width*.31f,height+baseY+.68f,z-depth*.28f,
               width*.22f,.75f,depth*.24f,facade,style==0?3.0f:0.0f);
    }
}

void CityWorld::addStreetFurniture(std::vector<Vertex>& out, float x, float z, std::uint32_t seed) {
    const float darkMetal[3]={0.12f,0.15f,0.17f};
    const float warmLight[3]={1.0f,0.77f,0.48f};
    const float concrete[3]={0.43f,0.43f,0.40f};
    addBox(out,x,2.45f,z,.11f,4.90f,.11f,darkMetal,4);
    addBox(out,x+.42f,4.78f,z,.90f,.09f,.10f,darkMetal,4);
    addBox(out,x+.78f,4.64f,z,.28f,.12f,.22f,warmLight,7);
    // Seat, back and supports are grouped in the same chunk mesh.
    if ((hash(seed)%3u)==0u) {
        addBox(out,x-1.45f,.48f,z+1.2f,1.75f,.12f,.55f,concrete,0);
        addBox(out,x-1.45f,.86f,z+1.45f,1.75f,.62f,.12f,darkMetal,4);
        for (int leg : {-1,1})
            addBox(out,x-1.45f+leg*.66f,.24f,z+1.2f,.10f,.48f,.12f,darkMetal,4);
    }
}

void CityWorld::addChunkGround(std::vector<Vertex>& out, int chunkX, int chunkZ, bool plaza) {
    const float ox=chunkX*kChunkSize, oz=chunkZ*kChunkSize;
    const float concrete[3]={0.40f,0.42f,0.41f};
    const float asphalt[3]={0.13f,0.15f,0.16f};
    const float sidewalk[3]={0.56f,0.56f,0.52f};
    const float curb[3]={0.67f,0.66f,0.61f};
    const float paint[3]={0.84f,0.84f,0.77f};
    auto ground=[&](float x0,float z0,float x1,float z1,const float* color,float material,float elevation=0.0f) {
        const float a[3]={ox+x0,elevation,oz+z0};
        const float b[3]={ox+x0,elevation,oz+z1};
        const float c[3]={ox+x1,elevation,oz+z1};
        const float d[3]={ox+x1,elevation,oz+z0};
        addQuad(out,a,b,c,d,color,material,std::max(1.0f,(x1-x0)*.24f),std::max(1.0f,(z1-z0)*.24f));
    };
    {
        const float a[3]={ox, -0.06f, oz};
        const float b[3]={ox, -0.06f, oz+kChunkSize};
        const float c[3]={ox+kChunkSize, -0.06f, oz+kChunkSize};
        const float d[3]={ox+kChunkSize, -0.06f, oz};
        addQuad(out,a,b,c,d,concrete,plaza?8.0f:0.0f,20.0f,20.0f);
    }
    // Four road corridors continue seamlessly across chunk boundaries.
    ground(0,0,kChunkSize,9.0f,asphalt,1);
    ground(0,75.0f,kChunkSize,kChunkSize,asphalt,1);
    ground(0,9.0f,9.0f,75.0f,asphalt,1);
    ground(75.0f,9.0f,kChunkSize,75.0f,asphalt,1);
    ground(0,9.0f,kChunkSize,13.0f,sidewalk,0);
    ground(0,71.0f,kChunkSize,75.0f,sidewalk,0);
    ground(9.0f,13.0f,13.0f,71.0f,sidewalk,0);
    ground(71.0f,13.0f,75.0f,71.0f,sidewalk,0);
    // Curbs and tactile edges.
    addBox(out,ox+42.0f,.12f,oz+9.0f,84.0f,.24f,.22f,curb,0);
    addBox(out,ox+42.0f,.12f,oz+75.0f,84.0f,.24f,.22f,curb,0);
    addBox(out,ox+9.0f,.12f,oz+42.0f,.22f,.24f,84.0f,curb,0);
    addBox(out,ox+75.0f,.12f,oz+42.0f,.22f,.24f,84.0f,curb,0);
    // Dashed lane guides and short crosswalk bars at each intersection.
    for (int dash=0;dash<8;++dash) {
        const float dx=5.0f+dash*10.5f;
        ground(dx,4.33f,dx+4.5f,4.48f,paint,6,.014f);
        ground(dx,79.4f,dx+4.5f,79.55f,paint,6,.014f);
        const float dz=15.0f+dash*8.0f;
        ground(4.33f,dz,4.48f,dz+3.5f,paint,6,.014f);
        ground(79.4f,dz,79.55f,dz+3.5f,paint,6,.014f);
    }
    for (int stripe=0;stripe<6;++stripe) {
        const float x=33.0f+stripe*2.5f;
        ground(x,9.7f,x+1.25f,12.0f,paint,6,.014f);
        ground(x,72.0f,x+1.25f,74.3f,paint,6,.014f);
        ground(9.7f,33.0f+stripe*2.5f,12.0f,34.25f+stripe*2.5f,paint,6,.014f);
        ground(72.0f,33.0f+stripe*2.5f,74.3f,34.25f+stripe*2.5f,paint,6,.014f);
    }
}

void CityWorld::buildChunk(int chunkX, int chunkZ, int lod) {
    std::vector<Vertex> vertices;
    vertices.reserve(lod==0?85000u:30000u);
    const float ox=chunkX*kChunkSize, oz=chunkZ*kChunkSize;
    const std::uint32_t seed=hash(static_cast<std::uint32_t>(chunkX)*73856093u ^
                                  static_cast<std::uint32_t>(chunkZ)*19349663u ^ 0x9e3779b9u);
    const bool plaza=(chunkX==0 && chunkZ==0);
    addChunkGround(vertices,chunkX,chunkZ,plaza);
    const float grass[3]={0.20f,0.28f,0.19f};
    const float path[3]={0.54f,0.54f,0.50f};
    const float fountainStone[3]={0.56f,0.59f,0.58f};
    const float water[3]={0.16f,0.36f,0.42f};

    if (plaza) {
        // A pedestrian square is the spawn landmark, with a low fountain and seating.
        addBox(vertices,ox+42.0f,.18f,oz+42.0f,11.0f,.36f,11.0f,fountainStone,0);
        addBox(vertices,ox+42.0f,.42f,oz+42.0f,7.7f,.18f,7.7f,water,8);
        addBox(vertices,ox+42.0f,.78f,oz+42.0f,1.1f,.55f,1.1f,fountainStone,0);
        for (int side : {-1,1}) {
            addBox(vertices,ox+42.0f+side*12.0f,.30f,oz+31.0f,6.5f,.60f,.55f,fountainStone,0);
            addBox(vertices,ox+31.0f,.30f,oz+42.0f+side*12.0f,.55f,.60f,6.5f,fountainStone,0);
        }
        for (int i=0;i<8;++i) {
            const float a=i*2.0f*kPi/8.0f;
            addTree(vertices,ox+42.0f+std::cos(a)*25.0f,oz+42.0f+std::sin(a)*25.0f,
                    0.90f,seed+static_cast<std::uint32_t>(i));
        }
    }

    const int district=static_cast<int>(hash(seed+0xA51u)%5u);
    const bool greenBlock=!plaza && (hash(seed+0xB71u)%11u==0u);
    if (greenBlock) {
        // Occasional small park blocks give long walks a visual pause without breaking the street grid.
        for (int i=0;i<9;++i) {
            const float x=ox+18.0f+random01(seed+i*13u)*48.0f;
            const float z=oz+18.0f+random01(seed+i*37u)*48.0f;
            addTree(vertices,x,z,.80f+random01(seed+i*71u)*.45f,seed+i);
        }
        addBox(vertices,ox+42.0f,.025f,oz+42.0f,45.0f,.05f,.55f,path,0);
        addBox(vertices,ox+42.0f,.025f,oz+42.0f,.55f,.05f,45.0f,path,0);
    } else if (!plaza) {
        const float centers[2]={25.0f,59.0f};
        for (int row=0;row<2;++row) for (int col=0;col<2;++col) {
            const std::uint32_t lotSeed=hash(seed+static_cast<std::uint32_t>(row*17+col*29));
            const float cx=ox+centers[col]+(random01(lotSeed)-.5f)*2.8f;
            const float cz=oz+centers[row]+(random01(lotSeed+11u)-.5f)*2.8f;
            const int style=static_cast<int>((district+row+col)%5);
            const float distanceFromCore=std::sqrt(static_cast<float>(chunkX*chunkX+chunkZ*chunkZ));
            float height=style==2?38.0f: (style==0?18.0f:26.0f);
            height += random01(lotSeed+0x71u)*16.0f;
            if (distanceFromCore<2.5f) height+=18.0f;
            if (distanceFromCore>5.0f) height*=.72f;
            if (lod>0) height=std::min(height,45.0f);
            const float width=style==2?19.5f:17.5f;
            const float depth=style==2?19.0f:18.5f;
            addBuilding(vertices,cx,cz,width,depth,height,style,lotSeed,lod==0);
        }
    }

    // Continuous street rhythm: lamp standards, planted verge pockets, benches and parked cars.
    const float dark[3]={0.12f,0.15f,0.17f};
    const float white[3]={0.78f,0.79f,0.76f};
    const float rubber[3]={0.045f,0.05f,0.055f};
    for (int side : {-1,1}) {
        for (int point=0;point<2;++point) {
            const float px=ox+(point==0?16.0f:68.0f);
            const float pz=oz+(side<0?10.4f:73.6f);
            addStreetFurniture(vertices,px,pz,seed+static_cast<std::uint32_t>(point*7+side*13));
            const float tx=ox+(point==0?15.0f:69.0f);
            const float tz=oz+(side<0?17.0f:67.0f);
            if (!plaza && !greenBlock && lod==0)
                addTree(vertices,tx,tz,.72f,seed+static_cast<std::uint32_t>(point*23+side*31));
        }
    }

    // One or two curbside cars per block, with distinct paint, glass, lights and rubber.
    if (lod==0) {
        for (int car=0;car<1;++car) {
            const float cx=ox+35.0f+random01(seed+0x921u)*12.0f;
            const float cz=oz+6.1f;
            const float paint[3]={0.18f+random01(seed+0x11u)*.40f,
                                  0.20f+random01(seed+0x21u)*.38f,
                                  0.24f+random01(seed+0x31u)*.38f};
            addBox(vertices,cx,.74f,cz,6.4f,1.0f,2.25f,paint,4);
            addBox(vertices,cx,.13f,cz,6.7f,.18f,2.32f,dark,4);
            addBox(vertices,cx,.88f,cz,3.6f,.78f,1.82f,
                   (hash(seed+0x41u)&1u)?dark:white,2);
            for (int wheel : {-1,1}) for (int axle : {-1,1})
                addBox(vertices,cx+axle*2.05f,.36f,cz+wheel*1.0f,.72f,.72f,.25f,rubber,4);
            const float lamp[3]={0.96f,0.84f,0.62f};
            addBox(vertices,cx+3.18f,.76f,cz-.70f,.06f,.23f,.30f,lamp,7);
            addBox(vertices,cx+3.18f,.76f,cz+.70f,.06f,.23f,.30f,lamp,7);
        }
    }

    // Sidewalk bollards and a compact bus-stop / street-sign cluster.
    for (int i=0;i<3;++i) {
        const float bx=ox+22.0f+i*20.0f;
        addBox(vertices,bx,.58f,oz+11.2f,.12f,1.16f,.12f,dark,4);
    }
    if (lod==0 && hash(seed+0xF12u)%3u==0u) {
        addBox(vertices,ox+62.0f,1.28f,oz+11.5f,2.4f,2.35f,.15f,dark,4);
        addBox(vertices,ox+62.0f,1.33f,oz+11.39f,2.1f,1.95f,.035f,
               (hash(seed+0x712u)&1u)?water:white,2);
        addBox(vertices,ox+62.0f,.15f,oz+11.5f,2.7f,.30f,.65f,dark,4);
    }

    // Convert this whole block into one GPU batch: one draw per block, not one per prop.
    Chunk chunk{};
    chunk.x=chunkX; chunk.z=chunkZ; chunk.lod=lod;
    chunk.vertexCount=static_cast<GLsizei>(vertices.size());
    glGenVertexArrays(1,&chunk.vao);
    glGenBuffers(1,&chunk.vbo);
    glBindVertexArray(chunk.vao);
    glBindBuffer(GL_ARRAY_BUFFER,chunk.vbo);
    glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(vertices.size()*sizeof(Vertex)),
                 vertices.data(),GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,px)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,nx)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,u)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,r)));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4,1,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,material)));
    glBindVertexArray(0);
    chunks_[chunkKey(chunkX,chunkZ)]=chunk;
}

bool CityWorld::collides(float worldX, float worldZ) const {
    const int chunkX=static_cast<int>(std::floor(worldX/kChunkSize));
    const int chunkZ=static_cast<int>(std::floor(worldZ/kChunkSize));
    const float localX=worldX-chunkX*kChunkSize;
    const float localZ=worldZ-chunkZ*kChunkSize;
    const std::uint32_t seed=hash(static_cast<std::uint32_t>(chunkX)*73856093u ^
                                  static_cast<std::uint32_t>(chunkZ)*19349663u ^ 0x9e3779b9u);
    const bool plaza=chunkX==0 && chunkZ==0;
    const bool greenBlock=!plaza && (hash(seed+0xB71u)%11u)==0u;

    if(plaza) {
        // Keep the fountain basin and the planted trees solid, but leave the square open.
        if(std::fabs(localX-42.0f)<6.0f && std::fabs(localZ-42.0f)<6.0f) return true;
        for(int i=0;i<8;++i) {
            const float angle=i*2.0f*kPi/8.0f;
            const float tx=42.0f+std::cos(angle)*25.0f;
            const float tz=42.0f+std::sin(angle)*25.0f;
            const float dx=localX-tx,dz=localZ-tz;
            if(dx*dx+dz*dz<2.0f*2.0f) return true;
        }
        return false;
    }

    // Parks use the same deterministic placement as their generated tree meshes.
    if(greenBlock) {
        for(int i=0;i<9;++i) {
            const float tx=18.0f+random01(seed+i*13u)*48.0f;
            const float tz=18.0f+random01(seed+i*37u)*48.0f;
            const float scale=.80f+random01(seed+i*71u)*.45f;
            const float dx=localX-tx,dz=localZ-tz;
            const float radius=scale*.78f+.35f;
            if(dx*dx+dz*dz<radius*radius) return true;
        }
        return false;
    }

    // Match each lot's procedural jitter and footprint so the invisible collider
    // follows the visible building instead of an approximate, offset rectangle.
    const int district=static_cast<int>(hash(seed+0xA51u)%5u);
    const float centers[2]={25.0f,59.0f};
    for(int row=0;row<2;++row) for(int col=0;col<2;++col) {
        const std::uint32_t lotSeed=hash(seed+static_cast<std::uint32_t>(row*17+col*29));
        const float cx=centers[col]+(random01(lotSeed)-.5f)*2.8f;
        const float cz=centers[row]+(random01(lotSeed+11u)-.5f)*2.8f;
        const int style=static_cast<int>((district+row+col)%5);
        const float width=style==2?19.5f:17.5f;
        const float depth=style==2?19.0f:18.5f;
        if(std::fabs(localX-cx)<width*.5f+.55f &&
           std::fabs(localZ-cz)<depth*.5f+.55f) return true;
    }

    // The curbside sedan and two planted street trees occupy the same spots as their meshes.
    const float carX=35.0f+random01(seed+0x921u)*12.0f;
    if(std::fabs(localX-carX)<3.45f && std::fabs(localZ-6.1f)<1.35f) return true;
    for(int side : {-1,1}) for(int point=0;point<2;++point) {
        const float tx=point==0?15.0f:69.0f;
        const float tz=side<0?17.0f:67.0f;
        const float dx=localX-tx,dz=localZ-tz;
        if(dx*dx+dz*dz<1.35f*1.35f) return true;
    }

    return false;
}

void CityWorld::render(const float* viewProjection, float cameraX, float cameraY, float cameraZ) {
    if (!initialized_) return;
    const int newCenterX=static_cast<int>(std::floor(cameraX/kChunkSize));
    const int newCenterZ=static_cast<int>(std::floor(cameraZ/kChunkSize));
    centerChunkX_=newCenterX;
    centerChunkZ_=newCenterZ;

    // Keep a 5x5 neighborhood resident. The inner 3x3 uses window-level facade detail;
    // the outer ring uses broad bands and reduced height/detail. Chunks are deterministic.
    for (auto it=chunks_.begin();it!=chunks_.end();) {
        const int dx=it->second.x-centerChunkX_;
        const int dz=it->second.z-centerChunkZ_;
        if (std::abs(dx)>2 || std::abs(dz)>2) {
            if (it->second.vao) glDeleteVertexArrays(1,&it->second.vao);
            if (it->second.vbo) glDeleteBuffers(1,&it->second.vbo);
            it=chunks_.erase(it);
        } else {
            ++it;
        }
    }
    for (int dz=-2;dz<=2;++dz) for (int dx=-2;dx<=2;++dx) {
        const int cx=centerChunkX_+dx, cz=centerChunkZ_+dz;
        const auto key=chunkKey(cx,cz);
        const int desiredLod=(std::abs(dx)<=1 && std::abs(dz)<=1)?0:1;
        auto existing=chunks_.find(key);
        if(existing!=chunks_.end() && existing->second.lod!=desiredLod) {
            if(existing->second.vao) glDeleteVertexArrays(1,&existing->second.vao);
            if(existing->second.vbo) glDeleteBuffers(1,&existing->second.vbo);
            chunks_.erase(existing);
            existing=chunks_.end();
        }
        if(existing==chunks_.end()) buildChunk(cx,cz,desiredLod);
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);

    // Atmospheric dome tracks the camera but does not cast shadows or require a cubemap.
    const float skyScale=300.0f;
    const float skyModel[16] = {
        skyScale,0,0,0, 0,skyScale,0,0, 0,0,skyScale,0,
        cameraX,cameraY-0.6f,cameraZ,1
    };
    float skyMvp[16]{};
    // Column-major multiplication: viewProjection * skyModel.
    for (int col=0;col<4;++col) for (int row=0;row<4;++row) {
        for (int k=0;k<4;++k) skyMvp[col*4+row]+=viewProjection[k*4+row]*skyModel[col*4+k];
    }
    glUseProgram(skyProgram_);
    const GLint skyMvpLoc=glGetUniformLocation(skyProgram_,"uViewProjection");
    glUniformMatrix4fv(skyMvpLoc,1,GL_FALSE,skyMvp);
    glDepthMask(GL_FALSE);
    glBindVertexArray(skyVao_);
    glDrawArrays(GL_TRIANGLES,0,skyVertexCount_);
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);

    glUseProgram(program_);
    glUniformMatrix4fv(mvpLocation_,1,GL_FALSE,viewProjection);
    glUniform3f(cameraLocation_,cameraX,cameraY,cameraZ);
    glUniform3f(sunLocation_,-0.42f,0.84f,-0.34f);
    for (const auto& entry:chunks_) {
        const Chunk& chunk=entry.second;
        glBindVertexArray(chunk.vao);
        glDrawArrays(GL_TRIANGLES,0,chunk.vertexCount);
    }
    glBindVertexArray(0);
    glUseProgram(0);
    glDisable(GL_DEPTH_TEST);
}
