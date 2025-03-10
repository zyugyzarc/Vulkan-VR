#ifndef IMAGE_H
#define IMAGE_H

#include "vklib.h"

namespace vk {

class Device;

// Represents a VkImage.
// Currently wraps a VkImage, and allows a
// VkImageView to ve created from it.
class Image {

    Device& device;
    VkImage image;
    VkImageView imview = VK_NULL_HANDLE;
    VkDeviceMemory mem = VK_NULL_HANDLE;

    VkSampler _sampler = VK_NULL_HANDLE;

    void* _mapped_ptr = nullptr;
    uint32_t memsize;
public:
    
    // wrap an existsing image
    Image(Device& d, VkImage _) : device(d), image(_), mem(VK_NULL_HANDLE) {
        mem = VK_NULL_HANDLE;
    }

    // create a new image
    Image(Device& d, VkImageCreateInfo, VkMemoryPropertyFlags);

    // create a sampler
    VkSampler sampler();

    void* map();
    void unmap(void*&);

    template <typename func_t>
    void mapped(func_t);

    ~Image();

    // allow creation of imageViews
    void view(VkImageViewCreateInfo);
    void view(VkImageViewCreateInfo, bool);

    operator VkImage() {return image;}
    operator VkImageView() {return imview;}
};

template <typename func_t>
void Image::mapped(func_t func) {
    void* ptr = map();
    func(ptr);
    unmap(ptr);
}

};
#endif