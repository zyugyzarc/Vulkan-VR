#ifndef SHADER_CPP
#define SHADER_CPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <vector>

#ifndef HEADER
    #define HEADER
    #include "device.cpp"
    #undef HEADER
#else
    namespace vk {
        class Device;
    };
#endif

namespace vk {

// Represents a Shader.
// A shader can be a vert shader, a frag shader or a comp shader.
// Wraps a VkShaderModule.
// Upon construction with a filename and code, automatically
// puts the code in the file, compiles it with glslc, and 
// loads the spv into the shadermodule.
class ShaderModule {

    Device& device;
    VkShaderModule module;
    VkShaderStageFlagBits type;

public:
    
    #define SHADERCODE(...) #__VA_ARGS__

    ShaderModule(Device& d, std::string filename, std::string code);
    ~ShaderModule();

    operator VkShaderModule() const {return module;}
};

}; // end of instance.h file
#ifndef HEADER

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cstdlib>
#include <vulkan/vk_enum_string_helper.h>

namespace vk {
#define VK_ASSERT(x) { _VkAssert(x, __FILE__, __LINE__); }

inline void _VkAssert (VkResult res, std::string file, int line) {
    if (res != VK_SUCCESS)
        throw std::runtime_error( 
            std::string(string_VkResult(res))
         + " at "
         + file + ":"
         + std::to_string(line)
    );
}

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
#endif
#endif