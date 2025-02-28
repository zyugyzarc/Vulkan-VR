#ifndef COMMANDBUFFER_CPP
#define COMMANDBUFFER_CPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <string>

// in theory has no dependencies
#ifndef HEADER
    #define HEADER
    #include "pipeline.cpp"
    #include "image.cpp"
    #include "buffer.cpp"
    #undef HEADER
#else
    namespace vk{
        class Pipeline;
        class Buffer;
    }
#endif

namespace vk {

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
    void beginRendering(std::vector<VkRenderingAttachmentInfo> attachment_col,
                        VkRenderingAttachmentInfo attachment_depth, VkRect2D area);

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


#define VK_ASSERT(x) { _VkAssert(x, __FILE__, __LINE__); }
void _VkAssert (VkResult res, std::string file, int line);

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

}; // end of instance.h file
#ifndef HEADER

#include <stdexcept>
#include <vulkan/vk_enum_string_helper.h>

namespace vk {

void _VkAssert (VkResult res, std::string file, int line) {
    if (res != VK_SUCCESS)
        throw std::runtime_error( 
            std::string(string_VkResult(res))
         + " at "
         + file + ":"
         + std::to_string(line)
    );
}

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
#endif
#endif