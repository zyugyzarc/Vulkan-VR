#ifndef COMMANDBUFFER_H
#define COMMANDBUFFER_H

#include "vklib.h"

namespace vk{

class Pipeline;
class Buffer;
class RenderPass;
    
// Wraps a VkCommandBuffer.
// Allocation is handled by Queue.
class CommandBuffer {

    VkCommandBuffer cmd;

    friend class Queue;
    CommandBuffer(VkCommandBuffer b) : cmd(b) {}

public:

    // Records a CommandBuffer.
    // func should be a callable that takes a CommandBuffer& as an argument.
    template <typename func_t>
    CommandBuffer& operator<< (func_t func);

    // mirrors vkCmdBeginRendering.
    // takes in a list of attachments.
    // void beginRendering(std::vector<VkRenderingAttachmentInfo> attachment_col,
    //                     VkRenderingAttachmentInfo attachment_depth, VkRect2D area);

    // mirrors vkCmdBeginRenderPass
    void beginRenderpass(RenderPass&, VkRect2D, std::vector<VkClearValue>);

    // mirrors vkCmdEndRenderPass
    void endRenderpass(RenderPass&);

    // mirrors vkCmdEndRendering
    void endRendering();

    // mirrors vkCmdBindPipeline
    void bindPipeline(Pipeline&);

    // mirrors vkCmdPushConstants
    void setPcrData(Pipeline&, int, void*);

    template <typename T>
    void setPcr(Pipeline&, int, T);

    // mirrors vkCmdBindVertexBuffers
    void bindVertexInput(std::vector<Buffer*>);

    // mirrors vkCmdBindIndexBuffer
    void bindVertexIndices(Buffer&, VkIndexType);

    // mirrors vkCmdDraw(num_verts, num_instances)
    void draw(uint32_t, uint32_t);

    // mirrors vkCmdDrawIndexed(num_idx, num_instances)
    void drawIndexed(uint32_t, uint32_t);

    // mirrors vkCmdDispatch(x, y, z)
    void dispatch(uint32_t, uint32_t, uint32_t);

    // wraps vkCmdSetViewport and vkCmdSetScissor
    void setRenderArea(VkViewport, VkRect2D);

    // wraps vkCmdImageBlit
    void blit(Image&, VkImageLayout, VkOffset3D, Image&, VkImageLayout, VkOffset3D, VkImageAspectFlags);

    // transitions an image from one VkImageLayout to another
    void imageTransition(
        Image&,
        VkImageLayout, VkPipelineStageFlags, VkAccessFlags,
        VkImageLayout, VkPipelineStageFlags, VkAccessFlags,
        VkImageAspectFlags
    );

    // getters
    operator VkCommandBuffer() {return cmd;}
};

// Records a CommandBuffer
template <typename func_t>
CommandBuffer& CommandBuffer::operator<< (func_t func) {  // note: for some reason the template function needs to be in the header file

    // wipe the commandbuffer
    vkResetCommandBuffer(cmd, 0);

    // start recording
    VkCommandBufferBeginInfo beginInfo {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    VK_ASSERT( vkBeginCommandBuffer(cmd, &beginInfo) );

    // do everything the user wants
    func(*this);

    // stop recording
    VK_ASSERT( vkEndCommandBuffer(cmd) );
    return *this;
};

template <typename T>
void CommandBuffer::setPcr(Pipeline& p, int i , T d) {
    setPcrData(p, i, &d);
}

};
#endif