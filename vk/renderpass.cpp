#include "renderpass.h"

namespace vk {

/**
 * For each renderpass attachment, we need to have:
 * - VkFormat
 * - initial layout
 * - current layout
 * - final layout
 */


RenderPass::RenderPass(
    Device& d,
    VkPipelineBindPoint pipemode,
    std::vector<VkAttachmentDescription> input_attachments,
    std::vector<VkAttachmentDescription> color_attachments,
    VkAttachmentDescription depthstencil_attachment
) : device(d),
    _has_depth(depthstencil_attachment.format != VK_FORMAT_UNDEFINED),
    _num_col_attachments( color_attachments.size() ) {

    std::vector<VkAttachmentDescription> all_attachments;
    std::vector<VkAttachmentReference> color_attachref;
    std::vector<VkAttachmentReference> input_attachref;
    VkAttachmentReference depthstencil_attachref;

    uint32_t j = 0;

    for (auto i : input_attachments) {
        all_attachments.push_back({
            .format = i.format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = i.initialLayout == VK_IMAGE_LAYOUT_UNDEFINED ?
                      VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
            .initialLayout = i.initialLayout,
            .finalLayout = i.finalLayout
        });
        input_attachref.push_back({j, i.finalLayout});
        j++;
    }

    for (auto i : color_attachments) {
        all_attachments.push_back({
            .format = i.format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = i.initialLayout == VK_IMAGE_LAYOUT_UNDEFINED ?
                      VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
            .initialLayout = i.initialLayout,
            .finalLayout = i.finalLayout
        });
        color_attachref.push_back({j, i.finalLayout});
        j++;
    }

    if (depthstencil_attachment.format != VK_FORMAT_UNDEFINED)
        all_attachments.push_back({
            .format = depthstencil_attachment.format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = depthstencil_attachment.initialLayout == VK_IMAGE_LAYOUT_UNDEFINED ?
                      VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
            .initialLayout = depthstencil_attachment.initialLayout,
            .finalLayout = depthstencil_attachment.finalLayout
        });
        depthstencil_attachref = {j, depthstencil_attachment.finalLayout};


    VkSubpassDescription subpass {
        .pipelineBindPoint =  pipemode,
        .inputAttachmentCount = (uint32_t) input_attachref.size(),
        .pInputAttachments = input_attachref.data(),
        .colorAttachmentCount = (uint32_t) color_attachref.size(),
        .pColorAttachments = color_attachref.data(),
        .pDepthStencilAttachment = depthstencil_attachment.format == VK_FORMAT_UNDEFINED ?
                                   nullptr : &depthstencil_attachref
    };

    VkSubpassDependency deps[] = {
        {VK_SUBPASS_EXTERNAL, 0, VK_SHADER_STAGE_VERTEX_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT},
        {0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                 VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT},
    };

    VkRenderPassCreateInfo renderPassInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = (uint32_t) all_attachments.size(),
        .pAttachments = all_attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 2,
        .pDependencies = deps
    };

    VK_ASSERT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &pass));
}

RenderPass::~RenderPass () {
    vkDestroyRenderPass(device, pass, nullptr);
}

template <typename T>
std::vector<std::vector<T>> permute(const std::vector<std::vector<T>>& lists) {
    std::vector<std::vector<T>> result;
    std::vector<int> indices(lists.size(), 0);
    
    while (true) {
        std::vector<T> temp;
        for (size_t i = 0; i < lists.size(); i++) {
            temp.push_back(lists[i][indices[i]]);
        }
        result.push_back(temp);
        
        for (size_t i = 0; i < indices.size(); i++) {
            indices[i] = (indices[i] + 1) % lists[i].size();
        }
        
        bool completed = true;
        for(int i: indices){
            if (i != 0) completed = false; break;
        } 
        if (completed) break;
    }
    
    return result;
}

void RenderPass::framebuffers(std::vector<std::vector<VkImageView>> views) {

    fbidx = 0;

    std::vector<std::vector<VkImageView>> permutations = permute<VkImageView>(views);

    for (const auto& i : permutations) {

        VkFramebufferCreateInfo framebufferInfo {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = pass,
            .attachmentCount = (uint32_t) i.size(),
            .pAttachments = i.data(),
            .width = 1920, // TODO: ACTUALLY FIX THIS
            .height = 1080, // TODO: ACTUALLY FIX THIS
            .layers = 1, // TODO: ACTUALLY FIX THIS
        };

        VkFramebuffer fb;

        VK_ASSERT(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &fb));

        _framebuffers.push_back(fb);

    }

}

}