#ifndef DEVICE_CPP
#define DEVICE_CPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>

#ifndef HEADER
    #define HEADER
    #include "instance.cpp"
    #include "queue.cpp"
    #include "image.cpp"
    #include "buffer.cpp"
    #undef HEADER
#else
    namespace vk {
        class Instance;
        class Queue;
        class Image;
        class Buffer;
    };
#endif

// additional flag to the VkQueueFlagBits, to signal presentation support
// https://registry.khronos.org/vulkan/specs/latest/man/html/VkQueueFlagBits.html
#define VK_QUEUE_PRESENTATION_BIT 0x200

namespace vk {

// A vk::Device wraps a physical device and a VkDevice, and a VkSwapchainKHR.
// Also allows you to create Queues using Device::create_queue().
// The device is initialized and ready when Device::init() is called.
// Assumes the extensions VK_KHR_SWAPCHAIN_EXTENSION_NAME and VK_KHR_dynamic_rendering
// are available, and loads them.
class Device {

    const Instance& instance;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkSwapchainKHR swapchain;
    uint32_t swapindex;

    std::vector<Queue*> queues;
    std::vector<uint32_t> families;
    std::vector<Image*> swapimages;

    std::vector<VkSemaphore> sems;
    std::vector<VkFence> fences;

    void createswapchain();

    Queue* _stagerq;

public:
    // sole constructor
    Device(const Instance&);
    // starts up the device.
    void init();
    ~Device();

    // returns a queue with the specified flags from VkQueueFlagBits.
    // the queue is invalid until Device::init() is called.
    Queue& create_queue(uint32_t);

    // gets the next available image in the swapchain
    // can signal semaphore or fence when the image is available
    Image& getSwapchainImage(VkFence, VkSemaphore);

    // creates a semaphore
    VkSemaphore semaphore();

    // creates a fence
    VkFence fence();
    VkFence fence(bool);

    // waits for a fence
    void wait(VkFence);

    // waits till the device is done with everything
    void idle() {vkDeviceWaitIdle(device);};

    // helpers:
    void _copybuffer(Buffer& src, Buffer& dst);

    // getters
    operator VkPhysicalDevice() const {return physicalDevice;};
    operator VkDevice() const {return device;};
    operator VkSwapchainKHR() const {return swapchain;};
    uint32_t _swapimage_index() const {return swapindex;};
    const std::vector<uint32_t>& getqfs() const {return families;};
};

}; // end of device.h file
#ifndef HEADER

#include <stdexcept>
#include <string>
#include <set>
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

/**
 * Constructor - creates a device using instance
 * for now, picks the first available VkPhysicalDevice
 * the VkDevice is constructed later in Device::init()
 */
Device::Device (const Instance& instance) : instance(instance) {
    
    // look for physical devices
    VkInstance ins = (VkInstance) instance;

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(ins, &deviceCount, nullptr);

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(ins, &deviceCount, devices.data());

    // pick the first device for now
    physicalDevice = devices[0];

    // set null for late initialization
    device = NULL;

    // create a stager queue for internal use (_copybuffer)
    _stagerq = &create_queue(VK_QUEUE_TRANSFER_BIT);
}


// Creates a Queue with the specified flags. The Queue is active
// and usuable after Device::init() is called.
// This function should be called before Device::init().
Queue& Device::create_queue(uint32_t flags) {

    // fetch the available queuefamilies
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

    // split the flags into the real flags and the presentation flag
    bool present = flags & VK_QUEUE_PRESENTATION_BIT;
           flags = flags & ~VK_QUEUE_PRESENTATION_BIT;

    // try to find the correct queuefamily
    uint32_t qf = 0;
    for (int i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & flags == flags) {
            if (present) {
                VkBool32 p;
                vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, (VkSurfaceKHR) instance, &p);
                if (!p) continue;
            }

            qf = i;
        }
    }

    // construct the queue and store it for later
    Queue* ret = new Queue(*this, qf);
    queues.push_back(ret);
    return *ret;
}

// Starts up the device, by creating a VkDevice, and initializing all
// queues created with Device::create_queue()
void Device::init() {

    // validation layers
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        "VK_KHR_dynamic_rendering",
    };

    // run through the queues, and fetch their queuefamilies
    for (Queue* q : queues) {
        for (uint32_t i: families) {
            if (i == q->family) {
                goto skipinsert;
            }
        }
        families.push_back(q->family);
        skipinsert:
    }

    // create the QueueCreateInfos
    std::vector<VkDeviceQueueCreateInfo> qcinfos;
    const float f1 = 1.;

    for (uint32_t qf : families) {
        VkDeviceQueueCreateInfo qci {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = qf,
            .queueCount = 1,
            .pQueuePriorities = &f1
        };
        qcinfos.push_back(qci);
    }

    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamic_render {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR,
        .dynamicRendering = VK_TRUE,
    };

    // create the device
    VkPhysicalDeviceFeatures deviceFeatures{};
    VkDeviceCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &dynamic_render,
        .queueCreateInfoCount = (uint32_t) qcinfos.size(),
        .pQueueCreateInfos = qcinfos.data(),
        .enabledLayerCount = (uint32_t) validationLayers.size(),
        .ppEnabledLayerNames = validationLayers.data(),
        .enabledExtensionCount = (uint32_t) deviceExtensions.size(),
        .ppEnabledExtensionNames = deviceExtensions.data(),
        .pEnabledFeatures = &deviceFeatures,
    };

    VK_ASSERT( vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) );

    // init the queues
    for (Queue* q : queues) {
        q->init();
    }

    // create the swapchain
    createswapchain();
}

// Creates and initalizes the swapchain
// hardcoded to use 32-bit SRGB (vk_COLOR_FORMAT),
// VK_PRESENT_MODE_MAILBOX_KHR (not always available), and
// VK_COLOR_SPACE_SRGB_NONLINEAR_KHR.
// Also assumes that the image sharing mode is
// VK_SHARING_MODE_CONCURRENT between all queue families.
void Device::createswapchain() {

    // get the swapchain capabilities
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, instance, &capabilities);

    // 32-bit RGBA in sRGB
    VkSurfaceFormatKHR format {
        .format = vk_COLOR_FORMAT,
        .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
    };

    // present mode, assume mailbox exists. if errors, use VK_PRESENT_MODE_FIFO_KHR
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR; //VK_PRESENT_MODE_MAILBOX_KHR;

    // get the rendering extent from glfw
    int width, height;
    glfwGetFramebufferSize((GLFWwindow*) instance, &width, &height);
    VkExtent2D extent = {
        (uint32_t) width, (uint32_t) height
    };

    // should query vkGetPhysicalDeviceSurfaceCapabilitiesKHR, but 2 should be good enough
    uint32_t imageCount = capabilities.minImageCount + 1;

    VkSwapchainCreateInfoKHR createInfo {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = (VkSurfaceKHR) instance,
        .minImageCount = imageCount,
        .imageFormat = format.format,
        .imageColorSpace = format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = families.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = (uint32_t) families.size(),
        .pQueueFamilyIndices = families.data(),
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE,

    };

    VK_ASSERT( vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain) );

    // now, fetch the swapchain images
    std::vector<VkImage> _swapimages;
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
    _swapimages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, _swapimages.data());

    // and for each image, create a vk::Image
    for (VkImage i : _swapimages) {
        Image* img = new Image(*this, i);
        img->view( VkImageViewCreateInfo {
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = vk_COLOR_FORMAT,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            }
        });
        swapimages.push_back(img);
    }
}

// helper function to copy a buffer. called by Buffer::staged()
void Device::_copybuffer (Buffer& src, Buffer& dst) {
    _stagerq->command() << [&](CommandBuffer& cmd) {
        VkBufferCopy copyRegion {.size = src.getsize()};
        vkCmdCopyBuffer((VkCommandBuffer) cmd, src, dst, 1, &copyRegion);
    };
    _stagerq->submit(VK_NULL_HANDLE, {}, {}, {});
}

Image& Device::getSwapchainImage (VkFence f, VkSemaphore s) {
    vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, s, f, &swapindex);
    return *swapimages[swapindex];
}

VkSemaphore Device::semaphore() {
    VkSemaphoreCreateInfo i {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkSemaphore s;
    VK_ASSERT( vkCreateSemaphore(device, &i, nullptr, &s) );
    sems.push_back(s);
    return s;
}

VkFence Device::fence() {
    return fence(false);
}

VkFence Device::fence(bool signaled) {
    VkFenceCreateInfo i {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0u
    };
    VkFence s;
    VK_ASSERT( vkCreateFence(device, &i, nullptr, &s) );
    fences.push_back(s);
    return s;
}

void Device::wait (VkFence f) {
    vkWaitForFences(device, 1, &f, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &f);
}

// Destructor.
Device::~Device () {

    delete _stagerq;

    for (Image* i : swapimages) {
        delete i;
    }

    for (auto i : sems) {
        vkDestroySemaphore(device, i, nullptr);
    }

    for (auto i : fences) {
        vkDestroyFence(device, i, nullptr);
    }

    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroyDevice(device, nullptr);
}

};
#endif
#endif