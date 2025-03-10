#include "image.h"

namespace vk {

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

VkSampler Image::sampler() {

    if (_sampler != VK_NULL_HANDLE) {
        return _sampler;
    }

    VkSamplerCreateInfo samplerInfo {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .anisotropyEnable = VK_FALSE,
        .compareEnable = VK_FALSE,
        .unnormalizedCoordinates = VK_FALSE,
    };

    VK_ASSERT(vkCreateSampler(device, &samplerInfo, nullptr, &_sampler));

    return _sampler;
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