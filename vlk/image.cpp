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
// [!!] Currently, does not own the VkImage
class Image {

    Device& device;
    VkImage image;
    VkImageView imview = VK_NULL_HANDLE;
public:
    
    // for now, we can only wrap images that already exist
    Image(Device& d, VkImage _) : device(d), image(_) {}
    ~Image();

    // allow creation of imageViews
    void view(VkImageViewCreateInfo);
    void view(VkImageViewCreateInfo, bool);

    operator VkImageView() {return imview;}
};

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

void Image::view (VkImageViewCreateInfo v) { view(v, false); }

void Image::view (VkImageViewCreateInfo v, bool keep_prev) {
    if (imview != VK_NULL_HANDLE && !keep_prev) {
        vkDestroyImageView((VkDevice) device, imview, nullptr);
    }

    v.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    v.image = image;

    VK_ASSERT( vkCreateImageView(device, &v, nullptr, &imview) );
}

Image::~Image () {
    if (imview != VK_NULL_HANDLE) {
        vkDestroyImageView((VkDevice) device, imview, nullptr);
    }
}

};
#endif
#endif