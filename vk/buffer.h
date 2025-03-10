#ifndef BUFFER_H
#define BUFFER_H

class Device;

#include "vklib.h"

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

};
#endif