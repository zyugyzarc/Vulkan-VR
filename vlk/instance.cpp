#ifndef INSTANCE_CPP
#define INSTANCE_CPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace vk {

// an Instance wraps a VkInstance, a GLFWWindow and a Surface.
// upon initialization, automatically creates a GLFWwindow and a
// VkInstance.
class Instance {

    GLFWwindow* window;
    VkInstance instance;
    VkSurfaceKHR surface;

public:
    // sole constructor
    Instance();
    ~Instance();

    // updates the screen:
    //  - polls GLFW events and handles window events
    bool update();

    // getters
    operator VkSurfaceKHR() const {return surface;}
    operator VkInstance() const {return instance;}
    operator GLFWwindow*() const {return window;}
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

// sole constructor
// creates a window, vkinstance, and a VkSurfaceKHR
Instance::Instance () {

    // create the window
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // select the last monitor
    GLFWmonitor* fullscreen = NULL;
    #ifdef FULLSCREEN
    int num_monitors;
    GLFWmonitor** glfwmonitors = glfwGetMonitors(&num_monitors);
    fullscreen = glfwmonitors[num_monitors - 1];
    #endif

    window = glfwCreateWindow(1920, 1080, "Amogug", fullscreen, nullptr);

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
        .pApplicationName = "Funny VR thing",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "En Jin",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0,
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


// updates the window. returns false when the window is closed.
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
#endif
#endif