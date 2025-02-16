#ifndef BUFFER_CPP
#define BUFFER_CPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <vector>

#ifndef HEADER
    #define HEADER
    #include "device.cpp"
    #include "commandbuffer.cpp"
    #undef HEADER
#else
    #include "commandbuffer.cpp"
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

    void* _mapped_ptr = nullptr;

    void _copy_from_buffer(Buffer&);
public:
    Buffer(Device&, VkBufferUsageFlags, VkMemoryPropertyFlags, uint32_t);
    ~Buffer();

    void* map();
    void unmap(void*&);

    template <typename func_t>
    void mapped(func_t);

    template <typename func_t>
    void staged(func_t);

    //getters
    operator VkBuffer() {return buffer;}
    uint32_t getsize() {return size;}
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

// creates a staging buffer, maps it and returns
// a void* to the mapped memory (in func). copies
// the staging buffer and deletes it after.
template <typename func_t>
void Buffer::staged(func_t func) {

    Buffer* b = new Buffer(device, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, size);
    b->mapped(func);
    _copy_from_buffer(*b);
    delete b;
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

void Buffer::_copy_from_buffer(Buffer& b) {
    device._copybuffer(b, *this);
}

// maps the memory held by this buffer
void* Buffer::map() {
    if (_mapped_ptr != nullptr) return _mapped_ptr;
    vkMapMemory(device, memory, 0, size, 0, &_mapped_ptr);
    return _mapped_ptr;
}

// unmaps (and flushes) the memory allocated by map()
void Buffer::unmap(void*& ptr) {
    vkUnmapMemory(device, memory);
    _mapped_ptr = nullptr;
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