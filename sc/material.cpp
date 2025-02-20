#ifndef MATERIAL_CPP
#define MATERIAL_CPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#ifndef HEADER
    #define HEADER
    #include "../vk/vklib.h"
    #include "mesh.cpp"
    #undef HEADER
#else
    namespace sc {
        struct Vertex;
    };
#endif

namespace sc {


// represents a material.
// contains a (graphics) Pipeline, made with
// a vert and frag shader.
class Material {

    vk::Device& device;
    vk::Pipeline* pipe;
    vk::ShaderModule* vs;
    vk::ShaderModule* fs;

public:
    Material(vk::Device& d, std::string name, std::string vscode, std::string fscode);
    ~Material();

    void bind(vk::CommandBuffer&);

    void descriptorSet(uint32_t);
    void writeDescriptor(uint32_t, uint32_t, vk::Buffer&, VkDescriptorType);
};

}; // end of instance.h file
#ifndef HEADER
namespace sc {

Material::Material (vk::Device& d, std::string name, std::string vscode, std::string fscode): device(d) {

    vs = new vk::ShaderModule(d, "_"+name+".vert", vscode),
    fs = new vk::ShaderModule(d, "_"+name+".frag", fscode),
    
    pipe = &vk::Pipeline::Graphics(
        d, 
        { // descriptor inputs
            {{.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .stageFlags = VK_SHADER_STAGE_ALL}},
            // single uniform for everything, for now
            // RGB textures in the future
        },
        {{ // vertex inputs
            .stride = sizeof(Vertex),
            .rate = VK_VERTEX_INPUT_RATE_VERTEX,
            .attr = {
                {.format=VK_FORMAT_R32G32B32_SFLOAT}, // position
                {.format=VK_FORMAT_R32G32B32_SFLOAT, .offset=offsetof(Vertex, norm)}, // normal
                {.format=VK_FORMAT_R32G32_SFLOAT,    .offset=offsetof(Vertex, uv)},  // texture coords
            }
        }},
        *vs, {vk_COLOR_FORMAT}, vk_DEPTH_FORMAT, *fs
    );

    if (pipe == nullptr) {
        printf("[ERROR] bald 1\n");
    }
}
void Material::bind (vk::CommandBuffer& cmd) {

    if (pipe == nullptr) {
        printf("[ERROR] bald 2\n");
    }

    cmd.bindPipeline(*pipe);
}

void Material::descriptorSet(uint32_t i) {

    if (pipe == nullptr) {
        printf("[ERROR] bald 3\n");
    }

    pipe->descriptorSet(i);
}

void Material::writeDescriptor(uint32_t i, uint32_t j, vk::Buffer& b, VkDescriptorType t) {

    if (pipe == nullptr) {
        printf("[ERROR] bald 4\n");
    }

    pipe->writeDescriptor(i, j, b, t);
}

Material::~Material() {
    delete pipe;
    delete vs;
    delete fs;
}

};
#endif
#endif