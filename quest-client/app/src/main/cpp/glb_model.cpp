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
#include <cctype>
#include <vector>
#include <zlib.h>

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

    // User-supplied furniture is stored gzip-compressed in the APK to reduce
    // install size. Existing uncompressed GLBs continue through unchanged.
    if(bytes.size()>=2 && bytes[0]==0x1f && bytes[1]==0x8b) {
        z_stream stream{};
        stream.next_in=bytes.data();
        stream.avail_in=static_cast<uInt>(bytes.size());
        if(inflateInit2(&stream,MAX_WBITS+16)!=Z_OK) {
            GLB_LOG("gzip init failed for %s",assetPath);
            return false;
        }
        std::vector<uint8_t> decoded;
        std::array<uint8_t,65536> chunk{};
        int inflateResult=Z_OK;
        while(inflateResult==Z_OK) {
            stream.next_out=chunk.data();
            stream.avail_out=static_cast<uInt>(chunk.size());
            inflateResult=inflate(&stream,Z_NO_FLUSH);
            const size_t produced=chunk.size()-stream.avail_out;
            decoded.insert(decoded.end(),chunk.begin(),chunk.begin()+produced);
            if(decoded.size()>32u*1024u*1024u) {
                inflateEnd(&stream);
                GLB_LOG("gzip model exceeds 32 MiB limit: %s",assetPath);
                return false;
            }
        }
        inflateEnd(&stream);
        if(inflateResult!=Z_STREAM_END || decoded.size()<12 ||
           std::memcmp(decoded.data(),"glTF",4)!=0) {
            GLB_LOG("gzip decompression failed for %s (zlib=%d)",assetPath,inflateResult);
            return false;
        }
        bytes.swap(decoded);
    }

    cgltf_options options{}; cgltf_data* data=nullptr;
    cgltf_result result=cgltf_parse(&options,bytes.data(),bytes.size(),&data);
    if(result!=cgltf_result_success || !data) { GLB_LOG("GLB parse failed (%d): %s",int(result),assetPath); return false; }
    // cgltf_parse reads the GLB container but does not attach the embedded BIN data
    // to buffer accessors. Load that buffer before unpacking positions or UVs.
    const cgltf_result bufferResult=cgltf_load_buffers(&options,data,nullptr);
    if(bufferResult!=cgltf_result_success) {
        GLB_LOG("GLB buffer load failed (%d): %s",int(bufferResult),assetPath);
        cgltf_free(data);
        return false;
    }
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

            auto uploadPart=[&](const std::vector<uint32_t>& sourceIndices,bool backrestPart) {
                if(sourceIndices.empty()) return;
                // Compact vertices per articulated part so the GPU buffers contain
                // only its triangles. The seat and wheels remain separate and static.
                std::vector<int32_t> remap(pos->count,-1);
                std::vector<float> partVertices;
                std::vector<uint32_t> partIndices;
                partVertices.reserve(sourceIndices.size()*5);
                partIndices.reserve(sourceIndices.size());
                for(uint32_t sourceIndex:sourceIndices) {
                    if(sourceIndex>=pos->count) continue;
                    if(remap[sourceIndex]<0) {
                        remap[sourceIndex]=static_cast<int32_t>(partVertices.size()/5);
                        for(int component=0;component<5;++component)
                            partVertices.push_back(vertices[static_cast<size_t>(sourceIndex)*5+component]);
                    }
                    partIndices.push_back(static_cast<uint32_t>(remap[sourceIndex]));
                }
                if(partIndices.empty()) return;
                Primitive p{};
                glGenVertexArrays(1,&p.vao); glGenBuffers(1,&p.vbo); glGenBuffers(1,&p.ebo);
                glBindVertexArray(p.vao); glBindBuffer(GL_ARRAY_BUFFER,p.vbo);
                glBufferData(GL_ARRAY_BUFFER,partVertices.size()*sizeof(float),partVertices.data(),GL_STATIC_DRAW);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,p.ebo);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER,partIndices.size()*sizeof(uint32_t),partIndices.data(),GL_STATIC_DRAW);
                glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,5*sizeof(float),nullptr);
                glEnableVertexAttribArray(1); glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,5*sizeof(float),reinterpret_cast<void*>(3*sizeof(float)));
                glBindVertexArray(0);
                p.indexCount=static_cast<GLsizei>(partIndices.size()); p.indexed=true;
                p.backrestPart=backrestPart;
                p.sourceMeshIndex=static_cast<uint32_t>(mi);
                p.sourcePrimitiveIndex=static_cast<uint32_t>(pi);
                if(src->material) {
                    const auto& mat=*src->material;
                    const auto& factor=mat.pbr_metallic_roughness.base_color_factor;
                    p.color={float(factor[0]),float(factor[1]),float(factor[2]),float(factor[3])};
                    const cgltf_texture_view* tv=&mat.pbr_metallic_roughness.base_color_texture;
                    if(tv->texture && tv->texture->image) p.texture=makeTexture(tv->texture->image);
                }
                p.nodeMatrix={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
                primitives_.push_back(p);
            };

            const bool chairAsset=std::strstr(assetPath,"white_gaming_chair.glb")!=nullptr;
            const bool splitBackrest=chairAsset&&mesh->name&&
                (std::strcmp(mesh->name,"chair_wl")==0||std::strcmp(mesh->name,"chair_piping")==0);
            if(splitBackrest) {
                std::vector<uint32_t> seatIndices,backrestIndices;
                seatIndices.reserve(indices.size());
                backrestIndices.reserve(indices.size()/2);
                for(size_t tri=0;tri+2<indices.size();tri+=3) {
                    const uint32_t a=indices[tri],b=indices[tri+1],c=indices[tri+2];
                    const float centerY=(positions[a*3+1]+positions[b*3+1]+positions[c*3+1])/3.0f;
                    const float centerZ=(positions[a*3+2]+positions[b*3+2]+positions[c*3+2])/3.0f;
                    // The backrest is the tall rear upholstered component in the source
                    // GLB's local coordinates: above the seat hinge and toward -Z.
                    auto& bucket=(centerY>.56f&&centerZ<-.13f)?backrestIndices:seatIndices;
                    bucket.push_back(a);bucket.push_back(b);bucket.push_back(c);
                }
                uploadPart(seatIndices,false);
                uploadPart(backrestIndices,true);
            } else {
                uploadPart(indices,false);
            }
        }
    }
    // Apply node transforms by duplicating each mesh primitive with its node matrix.
    // Most supplied models have one node per mesh; preserve the node hierarchy via world matrices.
    if(data->nodes_count) {
        std::vector<Primitive> transformed;
        for(cgltf_size ni=0;ni<data->nodes_count;++ni) {
            cgltf_node* node=&data->nodes[ni]; if(!node->mesh) continue;
            float world[16]; cgltf_node_transform_world(node,world);
            const uint32_t sourceMeshIndex=static_cast<uint32_t>(node->mesh-data->meshes);
            // A mesh can expand into multiple draw primitives (for example, the
            // seat and backrest split). Preserve the node transform on every part.
            for(const Primitive& base:primitives_) {
                if(base.sourceMeshIndex!=sourceMeshIndex) continue;
                Primitive copy=base;
                std::copy(world,world+16,copy.nodeMatrix.begin());
                copy.nodeName=node->name?node->name:"";
                transformed.push_back(copy);
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
                    GLint locTextured,GLint locTexture,GLint locMaterial,GLint locRounded,GLint locUvScale,
                    const std::array<float,4>* emissiveOverrideColor,const char* emissiveNodeMarker,
                    const std::array<float,4>* albedoTint,float seatLiftMeters,
                    float backrestReclineRadians,bool omitDisplaySurface) const {
    auto lowerName=[](std::string name) {
        std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
        return name;
    };
    auto isFixedBase=[&](const std::string& name) {
        const std::string lower=lowerName(name);
        return lower.find("wheel")!=std::string::npos || lower.find("caster")!=std::string::npos ||
               lower.find("base")!=std::string::npos || lower.find("piston")!=std::string::npos ||
               lower.find("gas_lift")!=std::string::npos || lower.find("gas lift")!=std::string::npos;
    };
    auto isBackrest=[&](const std::string& name) {
        const std::string lower=lowerName(name);
        return lower.find("backrest")!=std::string::npos || lower.find("back_rest")!=std::string::npos ||
               lower.find("upper_back")!=std::string::npos || lower.find("back_pad")!=std::string::npos;
    };
    const float c=std::cos(backrestReclineRadians),sn=std::sin(backrestReclineRadians);
    // Chair-space hinge sits just above the seat cushion, where the backrest meets the seat.
    // Measured chair hinge in the GLB's node/world coordinates. The source
    // nodes rotate 180 degrees around Y, so the hinge sits at +Z after node transform.
    const float hingeY=.52f+seatLiftMeters,hingeZ=.155f;
    const float toHinge[16]={1,0,0,0, 0,1,0,0, 0,0,1,0, 0,-hingeY,-hingeZ,1};
    const float fromHinge[16]={1,0,0,0, 0,1,0,0, 0,0,1,0, 0,hingeY,hingeZ,1};
    const float rotateBack[16]={1,0,0,0, 0,c,sn,0, 0,-sn,c,0, 0,0,0,1};
    float hingeTmp[16],hingeRotation[16];
    multiply(rotateBack,toHinge,hingeTmp);
    multiply(fromHinge,hingeTmp,hingeRotation);
    const float lift[16]={1,0,0,0, 0,1,0,0, 0,0,1,0, 0,seatLiftMeters,0,1};
    for(const Primitive& p:primitives_) {
        if(omitDisplaySurface) {
            const std::string node=lowerName(p.nodeName);
            if(node.find("screen")!=std::string::npos||node.find("display")!=std::string::npos||
               node.find("lcd")!=std::string::npos||node.find("glass")!=std::string::npos) continue;
        }
        const bool emissiveOverride=emissiveOverrideColor&&emissiveNodeMarker&&
            p.nodeName.find(emissiveNodeMarker)!=std::string::npos;
        std::array<float,4> color=emissiveOverride?*emissiveOverrideColor:p.color;
        if(albedoTint&&!emissiveOverride) {
            for(size_t channel=0;channel<4;++channel) color[channel]*=(*albedoTint)[channel];
        }
        float nodeTransform[16],model[16],mvp[16];
        std::copy(p.nodeMatrix.begin(),p.nodeMatrix.end(),nodeTransform);
        if(!isFixedBase(p.nodeName) && std::fabs(seatLiftMeters)>0.0001f) {
            float lifted[16]; multiply(lift,nodeTransform,lifted);
            std::copy(lifted,lifted+16,nodeTransform);
        }
        if((p.backrestPart||isBackrest(p.nodeName)) && std::fabs(backrestReclineRadians)>0.0001f) {
            float articulated[16]; multiply(hingeRotation,nodeTransform,articulated);
            std::copy(articulated,articulated+16,nodeTransform);
        }
        multiply(placement,nodeTransform,model); multiply(pv,model,mvp);
        glUniformMatrix4fv(locMvp,1,GL_FALSE,mvp); glUniformMatrix4fv(locModel,1,GL_FALSE,model);
        glUniform4fv(locColor,1,color.data());
        glUniform1i(locMaterial,emissiveOverride?7:-1); glUniform1i(locRounded,0);
        glUniform2f(locUvScale,1.0f,1.0f);
        const bool textured=p.texture&&!emissiveOverride;
        glUniform1i(locTextured,textured?1:0);
        if(textured) { glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,p.texture); glUniform1i(locTexture,0); }
        glBindVertexArray(p.vao); glDrawElements(GL_TRIANGLES,p.indexCount,GL_UNSIGNED_INT,nullptr); glBindVertexArray(0);
        if(textured) glBindTexture(GL_TEXTURE_2D,0);
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
