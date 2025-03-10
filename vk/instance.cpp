#include "instance.h"

namespace vk {

// sole constructor
// creates a window, vkinstance, and a VkSurfaceKHR
Instance::Instance (std::string windowname, int width, int height) {

    // create the window
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    // glfwWindowHint(GLFW_DECORATED, 0);

    // select the last monitor
    GLFWmonitor* fullscreen = NULL;

    int num_monitors;
    GLFWmonitor** glfwmonitors = glfwGetMonitors(&num_monitors);
    // fullscreen = glfwmonitors[num_monitors - 1];
    fullscreen = glfwmonitors[0];

    if (num_monitors <= 0) {
        throw std::runtime_error("no monitors");
    }

    const GLFWvidmode* mode = glfwGetVideoMode(fullscreen);

    width = (int)(mode->width * 2 + 250);
    height = (int)(mode->height * 2 + 100);

    this->width = width;
    this->height = height;

    window = glfwCreateWindow( width, height, windowname.c_str(), fullscreen, nullptr);

    printf("new window (%dx%d)\n", width, height);

    // glfwSetWindowMonitor(window, fullscreen, 0, 0, (width, (int)(mode->height * 1.25), mode->refreshRate);

    // get required extensions
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    // validation layers
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    // create the instance
    VkApplicationInfo appInfo {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = windowname.c_str(),
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = (windowname + "-engine").c_str(),
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_3,
    };

    VkInstanceCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(validationLayers.size()),
        .ppEnabledLayerNames = validationLayers.data(),
        .enabledExtensionCount = glfwExtensionCount,
        .ppEnabledExtensionNames = glfwExtensions,
    };

    VK_ASSERT( vkCreateInstance(&createInfo, nullptr, &instance) );

    // create surface
    VK_ASSERT( glfwCreateWindowSurface(instance, window, nullptr, &surface) );
}


// updates the window. returns false when the window needs to close.
bool Instance::update () {
    glfwPollEvents();
    return !glfwWindowShouldClose(window);
}

// destructor
Instance::~Instance () {

    // destroy surface
    vkDestroySurfaceKHR(instance, surface, nullptr);

    // destroy the instance
    vkDestroyInstance(instance, nullptr);

    // destroy the window
    glfwDestroyWindow(window);
    glfwTerminate();
}

};