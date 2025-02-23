#ifndef IMAGE_CPP
#define IMAGE_CPP

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

// Represents a VkImage.
// Currently wraps a VkImage, and allows a
// VkImageView to ve created from it.
class Image {

    Device& device;
    VkImage image;
    VkImageView imview = VK_NULL_HANDLE;
    VkDeviceMemory mem = VK_NULL_HANDLE;

    void* _mapped_ptr = nullptr;
    uint32_t memsize;
public:
    
    // wrap an existsing image
    Image(Device& d, VkImage _) : device(d), image(_), mem(VK_NULL_HANDLE) {
        mem = VK_NULL_HANDLE;
    }

    // create a new image
    Image(Device& d, VkImageCreateInfo, VkMemoryPropertyFlags);

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

}; // end of instance.h file
#ifndef HEADER

#include <stdexcept>
#include <string>
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

Image::Image (Device& d, VkImageCreateInfo info, VkMemoryPropertyFlags memflags) : device(d) {

    // all the parameters that need to be default
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.mipLevels = 1;
    info.arrayLayers = 1;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    info.samples = VK_SAMPLE_COUNT_1_BIT;

    // only the graphics queue needs to access this, so we're chilling
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_ASSERT( vkCreateImage(device, &info, nullptr, &image) );

    // now allocate memory
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);


    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(device, &memProperties);

    uint32_t i;
    for (i = 0; i < memProperties.memoryTypeCount; i++) {
        if (
            memRequirements.memoryTypeBits & (1 << i)
            && ((memProperties.memoryTypes[i].propertyFlags & memflags) == memflags)
        ) {
            goto found_heap_index;
        }
    }
    throw std::runtime_error("Failed to find device memory");
found_heap_index:

    memsize = memRequirements.size;

    VkMemoryAllocateInfo alloc {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = i
    };

    VK_ASSERT( vkAllocateMemory(device, &alloc, nullptr, &mem) );

    // ok cool, new image now
    vkBindImageMemory(device, image, mem, 0);
}


// maps the memory held by this image
void* Image::map() {
    if (_mapped_ptr != nullptr) return _mapped_ptr;
    vkMapMemory(device, mem, 0, memsize, 0, &_mapped_ptr);
    return _mapped_ptr;
}

// unmaps (and flushes) the memory allocated by map()
void Image::unmap(void*& ptr) {
    vkUnmapMemory(device, mem);
    _mapped_ptr = nullptr;
    ptr = nullptr;
}


void Image::view (VkImageViewCreateInfo v) { view(v, false); }

void Image::view (VkImageViewCreateInfo v, bool keep_prev) {
    if (imview != VK_NULL_HANDLE && !keep_prev) {
        vkDestroyImageView((VkDevice) device, imview, nullptr);
        imview = VK_NULL_HANDLE;
    }

    v.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    v.image = image;

    v.subresourceRange.levelCount = v.subresourceRange.levelCount == 0 ? 1 : v.subresourceRange.levelCount;
    v.subresourceRange.layerCount = v.subresourceRange.layerCount == 0 ? 1 : v.subresourceRange.layerCount;

    VK_ASSERT( vkCreateImageView(device, &v, nullptr, &imview) );
}

Image::~Image () {
    if (imview != VK_NULL_HANDLE) {
        vkDestroyImageView((VkDevice) device, imview, nullptr);
    }
    // printf("[DEBUG] is it owner? %d\n", owner);
    if (mem != VK_NULL_HANDLE) {
        vkDestroyImage(device, image, nullptr);
        vkFreeMemory(device, mem, nullptr);
    }
}

};
#endif
#endif