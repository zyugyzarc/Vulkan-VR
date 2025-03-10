#include <fstream>
#include "shadermodule.h"

namespace vk {

// helper -- check if `a` endswith `b`
static bool endswith (const std::string a, const std::string b) {

    if (a.size() < b.size()) return false;
    char* p = (char*) a.c_str();
    p += a.size() - b.size();
    std::string suffix(p);
    return p == b;
}

// Constructor - compiles a given shader program using glslc and creates a shadermodule
ShaderModule::ShaderModule(Device& d, std::string filename, std::string code) : device(d) {

    if (endswith(filename, ".vert")){
        this->type = VK_SHADER_STAGE_VERTEX_BIT;
    }
    else if (endswith(filename, ".frag")){
        this->type = VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    else if (endswith(filename, ".comp")){
        this->type = VK_SHADER_STAGE_COMPUTE_BIT;
    }

    std::ofstream glfile("/tmp/" + filename, std::ios::out);
    glfile << "#version 450\n";
    glfile << code;
    glfile.close();

    if ( 0 != 
        system(("./.util/glslc -g \"/tmp/" + filename + "\" -o \"/tmp/" + filename + ".spv\"").c_str())
    ) {
        throw std::runtime_error("shader compile failed");
    }

    std::ifstream spvfile("/tmp/" + filename + ".spv", std::ios::ate | std::ios::binary);

    size_t fileSize = (size_t) spvfile.tellg();
    std::vector<char> bcode(fileSize);
    spvfile.seekg(0);
    spvfile.read(bcode.data(), fileSize);
    spvfile.close();

    VkShaderModuleCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = bcode.size(),
        .pCode = (uint32_t*) bcode.data()
    };

    vkCreateShaderModule(device, &createInfo, nullptr, &module);
}

// destructor
ShaderModule::~ShaderModule () {
    vkDestroyShaderModule(device, module, nullptr);
}

};