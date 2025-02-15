#ifndef BUFFER_CPP
#define BUFFER_CPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
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

// Note: there's a better way to manage memory
//       by allocating bigger blocks at a time
//       and splitting them among buffers and images.
//       Thats too much work so I'm not doing it (for now)

// wraps a Buffer.
// allocates the relevant VkDeviceMemory.
class Buffer {

    Device& device;

    VkBuffer buffer;
    VkDeviceMemory memory;
    uint32_t size;

public:
    Buffer(Device&, VkBufferUsageFlags, VkMemoryPropertyFlags, uint32_t);
    ~Buffer();

    void* map();
    void unmap(void*&);

    template <typename func_t>
    void mapped(func_t);

    //getters
    operator VkBuffer() {return buffer;}
};

// creates a contextmanager for
// the mapped memory. func should 
// be a callable that takes in a void*
template <typename func_t>
void Buffer::mapped(func_t func) {
    void* ptr = map();
    func(ptr);
    unmap(ptr);
}

}; // end of instance.h file
#ifndef HEADER

#include <stdexcept>
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

// creates a buffer and allocates memory (of size)
Buffer::Buffer(Device& dev, VkBufferUsageFlags flags, VkMemoryPropertyFlags memflags, uint32_t size) : device(dev), size(size) {
    
    // create the buffer
    VkBufferCreateInfo bufferInfo {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = flags,
        .sharingMode = dev.getqfs().size() > 1 ? VK_SHARING_MODE_CONCURRENT
                                        : VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = (uint32_t) dev.getqfs().size(),
        .pQueueFamilyIndices = dev.getqfs().data()
    };

    VK_ASSERT( vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) );

    // allocate and init the memory

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(device, &memProperties);

    uint32_t i;
    for (i = 0; i < memProperties.memoryTypeCount; i++) {
        if (
            memRequirements.memoryTypeBits & (1 << i)
            && ((memProperties.memoryTypes[i].propertyFlags & memflags) == memflags)
        ) {
            // printf("[DEBUG] : using memory with %x == %x\n", memProperties.memoryTypes[i].propertyFlags, memflags);
            goto found_heap_index;
        }
    }
    throw std::runtime_error("Failed to find device memory");
found_heap_index:

    VkMemoryAllocateInfo allocInfo {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = i
    };

    VK_ASSERT( vkAllocateMemory(device, &allocInfo, nullptr, &memory) );

    vkBindBufferMemory(device, buffer, memory, 0);
}

// maps the memory held by this buffer
void* Buffer::map() {
    void* ret;
    vkMapMemory(device, memory, 0, size, 0, &ret);
    return ret;
}

// unmaps (and flushes) the memory allocated by map()
void Buffer::unmap(void*& ptr) {
    vkUnmapMemory(device, memory);
    ptr = nullptr;
}

// destructor
Buffer::~Buffer() {
    vkDestroyBuffer(device, buffer, nullptr);
    vkFreeMemory(device, memory, nullptr);
}

};
#endif
#endif