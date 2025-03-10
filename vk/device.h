#ifndef DEVICE_H
#define DEVICE_H

#include "vklib.h"

// additional flag to the VkQueueFlagBits, to signal presentation support
// https://registry.khronos.org/vulkan/specs/latest/man/html/VkQueueFlagBits.html
#define VK_QUEUE_PRESENTATION_BIT 0x200

namespace vk {

// forward declaration
class Instance;
class Queue;
class Image;
class Buffer;

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

};
#endif