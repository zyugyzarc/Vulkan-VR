#ifndef RENDERPASS_H
#define RENDERPASS_H

#include "vklib.h"

namespace vk {

class RenderPass {
    
    VkRenderPass pass;
    Device& device;

    uint32_t _num_col_attachments;
    bool _has_depth;

    std::vector<VkFramebuffer> _framebuffers;

    uint32_t fbidx;

public:

    RenderPass(Device& d, VkPipelineBindPoint, std::vector<VkAttachmentDescription>,
               std::vector<VkAttachmentDescription>, VkAttachmentDescription);
    ~RenderPass();

    void framebuffers(std::vector<std::vector<VkImageView>>);

    uint32_t num_col_attachments() const {return _num_col_attachments;}
    bool has_depth() const {return _has_depth;}

    VkFramebuffer get_current_fb() {
        auto rt = _framebuffers[fbidx];
        fbidx = (fbidx + 1) % _framebuffers.size();
        return rt;
    }

    // getters
    operator VkRenderPass() const {return pass;}
};

};
#endif