#ifndef QUEUE_CPP
#define QUEUE_CPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>

#ifndef HEADER
    #define HEADER
    #include "device.cpp"
    #include "commandbuffer.cpp"
    #include "image.cpp"
    #undef HEADER
#else
    namespace vk {
        class Device;
        class CommandBuffer;
        class Image;
    };
#endif

namespace vk {

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
#ifndef HEADER

#include <stdexcept>
#include <vector>
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

// constructor - everything is late initalization in Queue::init()
Queue::Queue (Device& d, uint32_t qf) : dev(d), family(qf) {}

// initalizes the queue -- fetches the correct VkQueue, sets up
// a command pool and a ring of command buffers.
// automatically called by Device::init()
void Queue::init () {
    // get the queue
    vkGetDeviceQueue((VkDevice) dev, family, 0, &queue);

    // create a commandpool
    VkCommandPoolCreateInfo poolInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = family
    };

    VK_ASSERT( vkCreateCommandPool(dev, &poolInfo, nullptr, &cmdPool) );

    // now allocate a ringbuffer of commandbuffers (wow)
    cmdbufs.resize(8); // 8 gotta be enough

    VkCommandBufferAllocateInfo allocInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = cmdPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 8,
    };

    VK_ASSERT( vkAllocateCommandBuffers(dev, &allocInfo, cmdbufs.data()) );

    for (auto i : cmdbufs) {
        cmdbufs_wrap.push_back(new CommandBuffer(i));
    }
}

// return the active command buffer. The command buffer cycles when submit() is called.
CommandBuffer& Queue::command() {
    // get the active commandbuffer
    auto ret = cmdbufs_wrap[curr_cmdbuf];
    // use the next one so you dont have to keep resetting the current cmdbuf
    curr_cmdbuf = (curr_cmdbuf + 1) % 8;
    return *ret;
}


// submits the active CommandBuffer, and cycles the ring of commandbuffers
// fence - fence is signaled when the operation is complete
// waitsems - semaphores to wait on before starting the operation
// waitstages - stages to wait at on the waitsems
// signalsems - semaphores to signal once operation is complete
void Queue::submit(CommandBuffer& cmd, VkFence f, std::vector<VkSemaphore> waitsem, std::vector<VkPipelineStageFlags> waitstage, std::vector<VkSemaphore> signalsem) {

    VkCommandBuffer c = cmd;

    VkSubmitInfo submit_info {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = (uint32_t) waitsem.size(),
        .pWaitSemaphores = waitsem.data(),
        .pWaitDstStageMask = waitstage.data(),
        .commandBufferCount = 1,
        .pCommandBuffers = &c,
        .signalSemaphoreCount = (uint32_t) signalsem.size(),
        .pSignalSemaphores = signalsem.data()
    };

    VK_ASSERT( vkQueueSubmit(queue, 1, &submit_info, f));
}

// submits the image fetched by `Device::getSwapchainImage()` to the 
// presentation engine. Optionally takes in the image itself.
// waitsems - semaphores to wait on before starting the operation
void Queue::present(Image& _, std::vector<VkSemaphore> waitsem) {

    VkSwapchainKHR swap = (VkSwapchainKHR)dev;
    uint32_t imageindex = dev._swapimage_index();

    VkPresentInfoKHR present {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = (uint32_t) waitsem.size(),
        .pWaitSemaphores = waitsem.data(),
        .swapchainCount = 1,
        .pSwapchains = &swap,
        .pImageIndices = &imageindex,
        .pResults = nullptr
    };

    vkQueuePresentKHR(queue, &present);
}

// destructor
Queue::~Queue () {
    vkDestroyCommandPool(dev, cmdPool, nullptr);
}

};
#endif
#endif