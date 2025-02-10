#ifndef DEVICE_CPP
#define DEVICE_CPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>

#ifndef HEADER
    #define HEADER
    #include "instance.cpp"
    #include "queue.cpp"
    #undef HEADER
#else
    namespace vk {
        class Instance;
        class Queue;
    };
#endif

// https://registry.khronos.org/vulkan/specs/latest/man/html/VkQueueFlagBits.html
#define VK_QUEUE_PRESENTATION_BIT 0x200

namespace vk {

// a Device wraps a physical device and a VkDevice, and a VkSwapchainKHR.
// also allows you to create a Queue, using create_queue.
//
class Device {

    const Instance& instance;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkSwapchainKHR swapchain;

    std::vector<Queue*> queues;

    void createswapchain();

public:
    // sole constructor
    Device(const Instance&);
    // starts up the device.
    void init();
    ~Device();

    // returns a queue with the specified flags from VkQueueFlagBits.
    // the queue is invalid until Device::init() is called.
    Queue& create_queue(uint32_t);

    // getters
    operator VkDevice() {return device;};
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
}

/**
 * Creates a Queue with the specified flags. To be called before
 * calling Device::init().
 */
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

/**
 * Starts up the device, by creating a VkDevice, and initializing all
 * queues created with create_queue
 */
void Device::init() {

    // validation layers
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    // run through the queues, and fetch their queuefamilies
    std::set<uint32_t> qfamilies;
    for (Queue* q : queues) {
        qfamilies.insert(q->family);
    }

    // create the QueueCreateInfos
    std::vector<VkDeviceQueueCreateInfo> qcinfos;
    const float f1 = 1.;

    for (uint32_t qf : qfamilies) {
        VkDeviceQueueCreateInfo qci {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = qf,
            .queueCount = 1,
            .pQueuePriorities = &f1
        };
        qcinfos.push_back(qci);
    }

    // create the device
    VkPhysicalDeviceFeatures deviceFeatures{};
    VkDeviceCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
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

/**
 * Creates and initalizes the swapchain
 */
void Device::createswapchain() {

    // get the swapchain capabilities
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, instance, &capabilities);

    // 32-bit RGBA in sRGB
    VkSurfaceFormatKHR format {
        .format = VK_FORMAT_B8G8R8A8_SRGB,
        .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
    };

    // present mode, assume mailbox exists. if errors, use VK_PRESENT_MODE_FIFO_KHR
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_MAILBOX_KHR;

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
        .imageArrayLayers = 1, // TODO: set to 2 for multi-view rendering
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE, // pretend that the graphics and present queue will be the same
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE,

    };

    VK_ASSERT( vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain) );
}

Device::~Device () {
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroyDevice(device, nullptr);
}

};
#endif
#endif