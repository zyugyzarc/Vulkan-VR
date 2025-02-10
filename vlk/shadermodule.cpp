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

// represents a Shader.
class ShaderModule {

    Device& device;
    VkShaderModule module;
public:
    
    #define SHADERCODE(...) #__VA_ARGS__

    ShaderModule(Device& d, std::string filename, std::string code);
    ~ShaderModule();
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

ShaderModule::ShaderModule(Device& d, std::string filename, std::string code) : device(d) {

    std::ofstream glfile("shaders/" + filename, std::ios::out);
    glfile << code;
    glfile.close();

    if ( 0 != 
        system(("./shaders/glslc \"" + filename + "\" -o \"" + filename + ".spv\"").c_str())
    ) {
        throw std::runtime_error("shader compile failed");
    }

    std::ifstream spvfile("shaders/" + filename + ".spv", std::ios::ate | std::ios::binary);

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

ShaderModule::~ShaderModule () {
    vkDestroyShaderModule(device, module, nullptr);
}

};
#endif
#endif