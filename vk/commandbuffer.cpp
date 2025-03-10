
#include "commandbuffer.h"

namespace vk {

// vkCmdBeginRendering
void CommandBuffer::beginRendering (std::vector<VkRenderingAttachmentInfo> attachment_col,
                                    VkRenderingAttachmentInfo attachment_depth,
                                    VkRect2D area) {
                                        
    for (auto& i : attachment_col) {
        i.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    }
    attachment_depth.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

    VkRenderingInfo renderInfo {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
        .renderArea = area,
        .layerCount = 1,
        .colorAttachmentCount = (uint32_t) attachment_col.size(),
        .pColorAttachments = attachment_col.data(),
        .pDepthAttachment = attachment_depth.imageView == VK_NULL_HANDLE ? nullptr : &attachment_depth,
    };

    vkCmdBeginRendering(cmd, &renderInfo);
}

void CommandBuffer::imageTransition(
    Image& im,
    VkImageLayout srcl, VkPipelineStageFlags srcs, VkAccessFlags srca,
    VkImageLayout dstl, VkPipelineStageFlags dsts, VkAccessFlags dsta,
    VkImageAspectFlags aspect
) {

    VkImageMemoryBarrier transition{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = srca,
        .dstAccessMask = dsta,
        .oldLayout = srcl,
        .newLayout = dstl,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = (VkImage) im,
        .subresourceRange = {
            .aspectMask = aspect,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    vkCmdPipelineBarrier(cmd, 
        srcs, dsts,
        0,
        0, nullptr,
        0, nullptr,
        1, &transition
    );

}

// vkCmdBindPipeline
void CommandBuffer::bindPipeline(Pipeline& p){
    std::vector<VkDescriptorSet> d = p._getdescset();
    vkCmdBindDescriptorSets(cmd, p, p, 0, (uint32_t) d.size(), d.data(), 0, nullptr);
    vkCmdBindPipeline(cmd, p, p);
}

// vkCmdBindVertexBuffers
void CommandBuffer::bindVertexInput(std::vector<Buffer*> bufs){
    
    std::vector<VkDeviceSize> offsets;
    std::vector<VkBuffer> vbufs;

    for (Buffer* b : bufs) {
        offsets.push_back(0);
        vbufs.push_back( (VkBuffer)*b);
    }

    vkCmdBindVertexBuffers(cmd, 0, bufs.size(), vbufs.data(), offsets.data());
}

// vkCmdPushConstants
void CommandBuffer::setPcrData(Pipeline& pipe, int index, void* data) {
    VkPushConstantRange pc = pipe._getpcr()[index];
    vkCmdPushConstants(cmd, pipe, pc.stageFlags, pc.offset, pc.size, data);
}

// vkCmdBindIndexBuffer
void CommandBuffer::bindVertexIndices(Buffer& buf, VkIndexType type) {
    vkCmdBindIndexBuffer(cmd, buf, 0, type);
}

// vkCmdSetViewport and vkCmdSetScissor
void CommandBuffer::setRenderArea(VkViewport v, VkRect2D s){
    vkCmdSetViewport(cmd, 0, 1, &v);
    vkCmdSetScissor(cmd, 0, 1, &s);
}

// vkCmdDraw
void CommandBuffer::draw(uint32_t verts, uint32_t inst) {
    vkCmdDraw(cmd, verts, inst, 0, 0);
}

// vkCmdDispatch
void CommandBuffer::dispatch(uint32_t x, uint32_t y, uint32_t z) {
    vkCmdDispatch(cmd, x, y, z);
}

// vkCmdDrawIndexed
void CommandBuffer::drawIndexed(uint32_t verts, uint32_t inst) {
    vkCmdDrawIndexed(cmd, verts, inst, 0, 0, 0);
}

// vkCmdImageBlit
void CommandBuffer::blit(Image& src, VkImageLayout srcl, VkOffset3D srcext, Image& dst, VkImageLayout dstl, VkOffset3D dstext, VkImageAspectFlags aspect) {
    VkImageBlit blt {
        .srcSubresource = {
            .aspectMask = aspect,
            .layerCount = 1
        },
        .srcOffsets = {
            {0, 0, 0}, srcext
        },
        .dstSubresource = {
            .aspectMask = aspect,
            .layerCount = 1
        },
        .dstOffsets = {
            {0, 0, 0}, dstext
        },
    };
    vkCmdBlitImage(
        cmd,
        src, srcl,
        dst, dstl,
        1, &blt,
        VK_FILTER_LINEAR
    );
}

// vkCmdEndRendering
void CommandBuffer::endRendering () {
    vkCmdEndRendering(cmd);
}

};