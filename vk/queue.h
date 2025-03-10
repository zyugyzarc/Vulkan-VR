#ifndef QUEUE_H
#define QUEUE_H

#include "vklib.h"

namespace vk {

class Device;
class CommandBuffer;
class Image;


// A queue wraps a VkQueue, and
// abstract CommandPools and CommandBuffers.
// A Queue is constructed from Device::create_queue()
// and is "valid" when Device::init() is called.
class Queue {

    uint32_t family;
    VkQueue queue;
    Device& dev;

    VkCommandPool cmdPool;
    std::vector<VkCommandBuffer> cmdbufs;
    std::vector<CommandBuffer*> cmdbufs_wrap;
    uint32_t curr_cmdbuf = 0;

    friend class Device;
    void init();
    Queue(Device&, uint32_t);

public:
    ~Queue();   

    CommandBuffer& command();
    void submit(CommandBuffer&, VkFence, std::vector<VkSemaphore>, std::vector<VkPipelineStageFlags>, std::vector<VkSemaphore>);
    void present(Image&, std::vector<VkSemaphore>);

};


}; // end of instance.h file
#endif