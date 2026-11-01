#pragma once

#include <GLES3/gl3.h>
#include <cstdint>
#include <unordered_map>
#include <vector>

class CityWorld {
public:
    bool initialize();
    void release();
    void render(const float* viewProjection, float cameraX, float cameraY, float cameraZ);
    bool collides(float worldX, float worldZ) const;

private:
    struct Vertex {
        float px, py, pz;
        float nx, ny, nz;
        float u, v;
        float r, g, b;
        float material;
    };
    struct Chunk {
        GLuint vao = 0;
        GLuint vbo = 0;
        GLsizei vertexCount = 0;
        int x = 0;
        int z = 0;
        int lod = 0;
    };

    static std::int64_t chunkKey(int x, int z);
    static std::uint32_t hash(std::uint32_t value);
    static float random01(std::uint32_t seed);
    void buildChunk(int chunkX, int chunkZ, int lod);
    void addQuad(std::vector<Vertex>& out, const float* a, const float* b,
                 const float* c, const float* d, const float* color,
                 float material, float repeatU = 1.0f, float repeatV = 1.0f);
    void addBox(std::vector<Vertex>& out, float x, float y, float z,
                float sx, float sy, float sz, const float* color,
                float material, float yaw = 0.0f);
    void addTree(std::vector<Vertex>& out, float x, float z, float scale, std::uint32_t seed);
    void addBuilding(std::vector<Vertex>& out, float x, float z, float width,
                     float depth, float height, int style, std::uint32_t seed, bool detailed);
    void addStreetFurniture(std::vector<Vertex>& out, float x, float z, std::uint32_t seed);
    void addChunkGround(std::vector<Vertex>& out, int chunkX, int chunkZ, bool plaza);

    GLuint program_ = 0;
    GLuint skyProgram_ = 0;
    GLuint skyVao_ = 0;
    GLuint skyVbo_ = 0;
    GLsizei skyVertexCount_ = 0;
    GLint mvpLocation_ = -1;
    GLint cameraLocation_ = -1;
    GLint sunLocation_ = -1;
    std::unordered_map<std::int64_t, Chunk> chunks_;
    int centerChunkX_ = 0;
    int centerChunkZ_ = 0;
    bool initialized_ = false;
};
