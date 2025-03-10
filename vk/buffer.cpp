#include "buffer.h"
namespace vk {

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