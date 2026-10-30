#define CGLTF_IMPLEMENTATION
#include "third_party/cgltf.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#include "third_party/stb_image.h"
#include "glb_model.h"
#include <android/asset_manager.h>
#include <android/log.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

#define GLB_LOG(...) __android_log_print(ANDROID_LOG_INFO,"QuestBridgeGLB",__VA_ARGS__)

namespace {
static void multiply(const float* a,const float* b,float* o) {
    for(int c=0;c<4;++c) for(int r=0;r<4;++r) {
        o[c*4+r]=0.0f;
        for(int k=0;k<4;++k) o[c*4+r]+=a[k*4+r]*b[c*4+k];
    }
}
static GLuint makeTexture(const cgltf_image* image) {
    if(!image || !image->buffer_view) return 0;
    const cgltf_buffer_view* view=image->buffer_view;
    const auto* bytes=static_cast<const stbi_uc*>(cgltf_buffer_view_data(view));
    if(!bytes || view->size==0) return 0;
    int w=0,h=0,channels=0;
    stbi_uc* pixels=stbi_load_from_memory(bytes,static_cast<int>(view->size),&w,&h,&channels,4);
    if(!pixels || w<=0 || h<=0) { if(pixels) stbi_image_free(pixels); return 0; }
    GLuint tex=0; glGenTextures(1,&tex); glBindTexture(GL_TEXTURE_2D,tex);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
    glGenerateMipmap(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D,0);
    stbi_image_free(pixels); return tex;
}
}

bool GlbModel::load(AAssetManager* assets,const char* assetPath) {
    release();
    if(!assets || !assetPath) return false;
    AAsset* asset=AAssetManager_open(assets,assetPath,AASSET_MODE_BUFFER);
    if(!asset) { GLB_LOG("asset missing: %s",assetPath); return false; }
    const off_t length=AAsset_getLength(asset);
    if(length<=0) { AAsset_close(asset); return false; }
    std::vector<uint8_t> bytes(static_cast<size_t>(length));
    const int read=AAsset_read(asset,bytes.data(),bytes.size()); AAsset_close(asset);
    if(read!=length) { GLB_LOG("short read for %s",assetPath); return false; }

    cgltf_options options{}; cgltf_data* data=nullptr;
    cgltf_result result=cgltf_parse(&options,bytes.data(),bytes.size(),&data);
    if(result!=cgltf_result_success || !data) { GLB_LOG("GLB parse failed (%d): %s",int(result),assetPath); return false; }
    // GLB's BIN chunk is retained by cgltf during parse. External buffers are deliberately rejected:
    // QuestBridge models are packaged as self-contained .glb assets.
    bool ok=true;
    for(cgltf_size mi=0;mi<data->meshes_count;++mi) {
        cgltf_mesh* mesh=&data->meshes[mi];
        for(cgltf_size pi=0;pi<mesh->primitives_count;++pi) {
            cgltf_primitive* src=&mesh->primitives[pi];
            const cgltf_accessor* pos=nullptr; const cgltf_accessor* uv=nullptr;
            for(cgltf_size ai=0;ai<src->attributes_count;++ai) {
                const auto& attr=src->attributes[ai];
                if(attr.type==cgltf_attribute_type_position) pos=attr.data;
                else if(attr.type==cgltf_attribute_type_texcoord && attr.index==0) uv=attr.data;
            }
            if(!pos || !pos->count) continue;
            std::vector<float> positions(pos->count*3);
            cgltf_accessor_unpack_floats(pos,positions.data(),positions.size());
            std::vector<float> uvs(pos->count*2,0.0f);
            if(uv) cgltf_accessor_unpack_floats(uv,uvs.data(),uvs.size());
            Primitive p{};
            std::vector<float> vertices(pos->count*5);
            for(cgltf_size v=0;v<pos->count;++v) {
                vertices[v*5+0]=positions[v*3+0]; vertices[v*5+1]=positions[v*3+1]; vertices[v*5+2]=positions[v*3+2];
                vertices[v*5+3]=uvs[v*2+0]; vertices[v*5+4]=uvs[v*2+1];
            }
            std::vector<uint32_t> indices;
            if(src->indices) {
                indices.resize(src->indices->count);
                for(cgltf_size i=0;i<src->indices->count;++i) indices[i]=static_cast<uint32_t>(cgltf_accessor_read_index(src->indices,i));
            } else {
                indices.resize(pos->count); for(cgltf_size i=0;i<pos->count;++i) indices[i]=static_cast<uint32_t>(i);
            }
            if(src->type!=cgltf_primitive_type_triangles) continue;
            glGenVertexArrays(1,&p.vao); glGenBuffers(1,&p.vbo); glGenBuffers(1,&p.ebo);
            glBindVertexArray(p.vao); glBindBuffer(GL_ARRAY_BUFFER,p.vbo);
            glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(float),vertices.data(),GL_STATIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,p.ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,indices.size()*sizeof(uint32_t),indices.data(),GL_STATIC_DRAW);
            glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,5*sizeof(float),nullptr);
            glEnableVertexAttribArray(1); glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,5*sizeof(float),reinterpret_cast<void*>(3*sizeof(float)));
            glBindVertexArray(0); p.indexCount=static_cast<GLsizei>(indices.size()); p.indexed=true;
            if(src->material) {
                const auto& mat=*src->material;
                const auto& factor=mat.pbr_metallic_roughness.base_color_factor;
                p.color={float(factor[0]),float(factor[1]),float(factor[2]),float(factor[3])};
                const cgltf_texture_view* tv=&mat.pbr_metallic_roughness.base_color_texture;
                if(tv->texture && tv->texture->image) p.texture=makeTexture(tv->texture->image);
            }
            p.nodeMatrix={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
            primitives_.push_back(p);
        }
    }
    // Apply node transforms by duplicating each mesh primitive with its node matrix.
    // Most supplied models have one node per mesh; preserve the node hierarchy via world matrices.
    if(data->nodes_count) {
        std::vector<Primitive> transformed;
        for(cgltf_size ni=0;ni<data->nodes_count;++ni) {
            cgltf_node* node=&data->nodes[ni]; if(!node->mesh) continue;
            float world[16]; cgltf_node_transform_world(node,world);
            for(cgltf_size pi=0;pi<node->mesh->primitives_count;++pi) {
                // Match the mesh primitive in traversal order; clone GPU handles are shared safely.
                size_t index=0;
                for(cgltf_size m=0;m<data->meshes_count;++m) {
                    if(&data->meshes[m]==node->mesh) break;
                    index+=data->meshes[m].primitives_count;
                }
                index+=pi;
                if(index<primitives_.size()) { Primitive copy=primitives_[index]; std::copy(world,world+16,copy.nodeMatrix.begin()); transformed.push_back(copy); }
            }
        }
        if(!transformed.empty()) primitives_.swap(transformed);
    }
    cgltf_free(data);
    if(primitives_.empty()) { GLB_LOG("no triangle meshes in %s",assetPath); ok=false; }
    GLB_LOG("loaded %s (%zu draw primitives)",assetPath,primitives_.size());
    return ok;
}

void GlbModel::draw(const float* pv,const float* placement,GLint locMvp,GLint locModel,GLint locColor,
                    GLint locTextured,GLint locTexture,GLint locMaterial,GLint locRounded,GLint locUvScale) const {
    for(const Primitive& p:primitives_) {
        float model[16],mvp[16]; multiply(placement,p.nodeMatrix.data(),model); multiply(pv,model,mvp);
        glUniformMatrix4fv(locMvp,1,GL_FALSE,mvp); glUniformMatrix4fv(locModel,1,GL_FALSE,model);
        glUniform4fv(locColor,1,p.color.data()); glUniform1i(locMaterial,-1); glUniform1i(locRounded,0);
        glUniform2f(locUvScale,1.0f,1.0f);
        glUniform1i(locTextured,p.texture?1:0);
        if(p.texture) { glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,p.texture); glUniform1i(locTexture,0); }
        glBindVertexArray(p.vao); glDrawElements(GL_TRIANGLES,p.indexCount,GL_UNSIGNED_INT,nullptr); glBindVertexArray(0);
        if(p.texture) glBindTexture(GL_TEXTURE_2D,0);
    }
}
void GlbModel::release() {
    for(auto& p:primitives_) {
        if(p.vao) glDeleteVertexArrays(1,&p.vao);
        if(p.vbo) glDeleteBuffers(1,&p.vbo);
        if(p.ebo) glDeleteBuffers(1,&p.ebo);
        if(p.texture) glDeleteTextures(1,&p.texture);
    }
    primitives_.clear();
}
