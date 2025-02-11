#ifndef PIPELINE_CPP
#define PIPELINE_CPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>

#ifndef HEADER
    #define HEADER
    #include "device.cpp"
    #include "shadermodule.cpp"
    #undef HEADER
#else
    namespace vk {
        class Device;
        class ShaderModule;
    };
#endif

namespace vk {

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
    VkDescriptorSetLayout desc_layout;
    VkPipelineBindPoint type;

    Pipeline(Device& d) : device(d) {}
    
    void init_graphics(std::vector<VkDescriptorSetLayoutBinding>, std::vector<struct VertexInputBinding>,
                        ShaderModule&, std::vector<VkFormat>, VkFormat, ShaderModule&);

    void init_compute(ShaderModule&);

public:

    // Factory function - Graphics: Creates a graphics pipeline with the given parameters
    static Pipeline& Graphics(
        Device& d,
        std::vector<VkDescriptorSetLayoutBinding> descriptors,
        std::vector<struct VertexInputBinding> vertex_input, ShaderModule& v,
        std::vector<VkFormat> attachment_col, VkFormat attachment_depth, ShaderModule& f
    ) {
        Pipeline* p = new Pipeline(d);
        p->type = VK_PIPELINE_BIND_POINT_GRAPHICS;

        p->init_graphics(
            descriptors,
            vertex_input, v,
            attachment_col, attachment_depth, f
        );

        return *p;
    };

    static Pipeline& Compute(Device& d, ShaderModule& c) {
        Pipeline* p = new Pipeline(d);  p->init_compute(c);  return *p;
    };

    ~Pipeline();

    // getters
    operator VkPipeline(){return pipeline;}
    operator VkPipelineBindPoint(){return type;}
};

}; // end of instance.h file
#ifndef HEADER

#include <stdexcept>
#include <string>
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

// helper function used by the factory function / named constructor
void Pipeline::init_graphics (
    std::vector<VkDescriptorSetLayoutBinding> descriptors,
    std::vector<struct VertexInputBinding> vertex_input, ShaderModule& vert,
    std::vector<VkFormat> attachment_col, VkFormat attachment_depth, ShaderModule& frag
) {

    // use default values for the states

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data(),
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembly {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    VkPipelineViewportStateCreateInfo viewportState {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    VkPipelineRasterizationStateCreateInfo rasterizer {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f,
   };

    VkPipelineMultisampleStateCreateInfo multisampling {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
    };


    // wow i forgot about this part
    VkPipelineColorBlendAttachmentState colorBlendAttachment {
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };
    
    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
    for (int i = 0; i < attachment_col.size(); i++) {
        colorBlendAttachments.push_back(colorBlendAttachment);
    }

    VkPipelineColorBlendStateCreateInfo colorBlending {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = (uint32_t) colorBlendAttachments.size(),
        .pAttachments = colorBlendAttachments.data(),
    };

    // load shadermodules

    VkPipelineShaderStageCreateInfo vert_st {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = (VkShaderModule) vert,
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo frag_st {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = (VkShaderModule) frag,
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo shaderStages[] = {vert_st, frag_st};

    // end defaults

    // the part where you add descriptors
    // Note: we make a single descriptor set layout for this pipeline to use
    //       and all the bindings are in the single set.

    for (uint32_t i = 0; i < descriptors.size(); i++) {
        descriptors[i].binding = i;
        descriptors[i].descriptorCount = 
            descriptors[i].descriptorCount == 0 ?
            1 : descriptors[i].descriptorCount;
    }

    VkDescriptorSetLayoutCreateInfo descset {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = (uint32_t) descriptors.size(),
        .pBindings = descriptors.data(),
    };

    VK_ASSERT( vkCreateDescriptorSetLayout(device, &descset, nullptr, &desc_layout) );
    
    // DEBUG
    
    // VkPipelineLayoutCreateInfo pipelineLayoutInfo {
    //     .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    //     .setLayoutCount = 1,
    //     .pSetLayouts = &desc_layout,
    // };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 0,
    };

    VK_ASSERT( vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) );

    // the part where you add vertex inputs
    // Note: we do this for every entry of the set vertexinputbindings

    std::vector<VkVertexInputBindingDescription> vert_bindings;
    std::vector<VkVertexInputAttributeDescription> vert_attrs;

    uint32_t i;
    for (const auto& bind : vertex_input) {
        
        vert_bindings.push_back( VkVertexInputBindingDescription{
            .binding = i,
            .stride = bind.stride,
            .inputRate = bind.rate
        });

        uint32_t j = 0;
        for (auto attr : bind.attr) {
            
            attr.location = j;
            attr.binding = i;
            vert_attrs.push_back(attr);
            j++;
        }
        i++;
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = (uint32_t) vert_bindings.size(),
        .pVertexBindingDescriptions = vert_bindings.data(),
        .vertexAttributeDescriptionCount = (uint32_t) vert_attrs.size(),
        .pVertexAttributeDescriptions = vert_attrs.data(),
    };

    // the part where you add attachments
    VkPipelineRenderingCreateInfoKHR pipeline_rendering_create_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
        .colorAttachmentCount = (uint32_t) attachment_col.size(),
        .pColorAttachmentFormats = attachment_col.data(),
        .depthAttachmentFormat = attachment_depth
    };

    // check if depth is available and update this
    VkPipelineDepthStencilStateCreateInfo depthstencil {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = attachment_depth == VK_FORMAT_UNDEFINED ? VK_FALSE : VK_TRUE,
        .depthWriteEnable = attachment_depth == VK_FORMAT_UNDEFINED ? VK_FALSE : VK_TRUE,
        .depthCompareOp = attachment_depth == VK_FORMAT_UNDEFINED ? VK_COMPARE_OP_ALWAYS : VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE
    };

    // finally, create the pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &pipeline_rendering_create_info,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = nullptr,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = pipelineLayout,
    };

    vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
}

void Pipeline::init_compute (ShaderModule& c) {
    // todo
}

// destructor
Pipeline::~Pipeline() {
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, desc_layout, nullptr);
}

};
#endif
#endif