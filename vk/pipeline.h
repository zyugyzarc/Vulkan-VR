#ifndef PIPELINE_H
#define PIPELINE_H

#include "vklib.h"

namespace vk {

class Device;
class ShaderModule;
class Buffer;
class Image;

// constant
const int _p_res_count = 2;

// helper struct
struct VertexInputBinding {
    uint32_t stride;
    VkVertexInputRate rate;
    std::vector<VkVertexInputAttributeDescription> attr;
};

// Represents a pipeline
// A pipeline either holds a vert shader and a frag shader or
// holds a compute shader.
// A pipeline should be able to "bind" to descriptors and attachments.
class Pipeline {

    Device& device;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    
    std::vector<VkDescriptorSetLayout> desc_layouts;

    VkPipelineBindPoint type;

    VkDescriptorPool descriptorPool;
    std::vector<std::vector<VkDescriptorSet>> descsets;
    std::vector<uint32_t> descset_index = std::vector<uint32_t>(_p_res_count);

    std::vector<VkPushConstantRange> pushconstantranges;

    Pipeline(Device& d) : device(d) {}
    
    void init_graphics(std::vector<std::vector<VkDescriptorSetLayoutBinding>>, std::vector<VkPushConstantRange>, std::vector<struct VertexInputBinding>,
                        ShaderModule&, std::vector<VkFormat>, VkFormat, ShaderModule&);

    void init_compute(std::vector<std::vector<VkDescriptorSetLayoutBinding>>, std::vector<VkPushConstantRange>, ShaderModule&);

public:

    // Factory function - Graphics: Creates a graphics pipeline with the given parameters
    static Pipeline& Graphics(
        Device& d,
        std::vector<std::vector<VkDescriptorSetLayoutBinding>> descriptors, std::vector<VkPushConstantRange> pushconst,
        std::vector<struct VertexInputBinding> vertex_input, ShaderModule& v,
        std::vector<VkFormat> attachment_col, VkFormat attachment_depth, ShaderModule& f
    ) {
        Pipeline* p = new Pipeline(d);
        p->type = VK_PIPELINE_BIND_POINT_GRAPHICS;

        p->init_graphics(
            descriptors, pushconst,
            vertex_input, v,
            attachment_col, attachment_depth, f
        );

        return *p;
    };

    static Pipeline& Compute(Device& d,std::vector<std::vector<VkDescriptorSetLayoutBinding>> descriptors,
                            std::vector<VkPushConstantRange> pushconst, ShaderModule& c) {
        Pipeline* p = new Pipeline(d); 
        p->type = VK_PIPELINE_BIND_POINT_COMPUTE;
        p->init_compute(descriptors, pushconst, c);
        return *p;
    };

    // return the current descriptor set
    std::vector<VkDescriptorSet> _getdescset();

    // return the pushconstant ranges
    std::vector<VkPushConstantRange> _getpcr() {return pushconstantranges;}

    void descriptorSet(uint32_t);
    void writeDescriptor(uint32_t, uint32_t, Buffer&, VkDescriptorType);
    void writeDescriptor(uint32_t, uint32_t, Image&, VkDescriptorType);

    ~Pipeline();

    // getters
    operator VkPipeline(){return pipeline;}
    operator VkPipelineLayout(){return pipelineLayout;}
    operator VkPipelineBindPoint(){return type;}
};

};
#endif