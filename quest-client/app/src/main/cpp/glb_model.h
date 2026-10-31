#pragma once
#include <GLES3/gl3.h>
#include <android/asset_manager.h>
#include <array>
#include <string>
#include <vector>

class GlbModel {
public:
    bool load(AAssetManager* assets, const char* assetPath);
    void draw(const float* projectionView, const float* placement,
              GLint locMvp, GLint locModel, GLint locColor,
              GLint locTextured, GLint locTexture, GLint locMaterial,
              GLint locRounded, GLint locUvScale,
              const std::array<float,4>* emissiveOverrideColor = nullptr,
              const char* emissiveNodeMarker = nullptr) const;
    void release();
    bool loaded() const { return !primitives_.empty(); }
private:
    struct Primitive {
        GLuint vao=0, vbo=0, ebo=0, texture=0;
        GLsizei indexCount=0;
        std::array<float,4> color{1,1,1,1};
        std::string nodeName;
        std::array<float,16> nodeMatrix{};
        bool indexed=false;
    };
    std::vector<Primitive> primitives_;
};
