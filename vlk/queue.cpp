#ifndef QUEUE_CPP
#define QUEUE_CPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>

#ifndef HEADER
    #define HEADER
    #include "device.cpp"
    #undef HEADER
#else
    namespace vk {
        class Device;
    };
#endif

namespace vk {

// a queue wraps a VkQueue
// also can create command buffers
class Queue {

    uint32_t family;
    VkQueue queue;
    Device& dev;

    VkCommandPool cmdPool;
    std::vector<VkCommandBuffer> cmdbufs;
    uint32_t curr_cmdbuf = 0;

    friend class Device;
    void init();
    Queue(Device&, uint32_t);

public:
    ~Queue();
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

Queue::Queue (Device& d, uint32_t qf) : dev(d), family(qf) {}

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
}

Queue::~Queue () {
    vkDestroyCommandPool(dev, cmdPool, nullptr);
}

};
#endif
#endif