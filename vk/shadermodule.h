#ifndef SHADER_H
#define SHADER_H

#include "vklib.h"

namespace vk {

class Device;

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

};
#endif