#ifndef QUEUE_CPP
#define QUEUE_CPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

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
class Queue {

    uint32_t family;
    VkQueue queue;
    Device& dev;

    friend class Device;
    void init();
    Queue(Device&, uint32_t);

public:
    // sole constructor, to be used privatley
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
    vkGetDeviceQueue((VkDevice) dev, family, 0, &queue);
}

};
#endif
#endif