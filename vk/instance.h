#ifndef VK_INSTANCE_H
#define VK_INSTANCE_H

#include "vklib.h"

#define vk_COLOR_FORMAT VK_FORMAT_B8G8R8A8_UNORM
#define vk_DEPTH_FORMAT VK_FORMAT_D32_SFLOAT

namespace vk {

// A vk::Instance wraps a VkInstance, a GLFWWindow and a VkSurfaceKHR.
// Upon initiazation, automatically constructs the required resources.
// (tries to) Open the window in fullscreen, on the last (secondary) monitor,
// with no window decoration.
class Instance {

    GLFWwindow* window;
    VkInstance instance;
    VkSurfaceKHR surface;

public:
    // sole constructor
    Instance(std::string, int, int);
    ~Instance();

    // other args (dont modify pls)
    uint32_t width;
    uint32_t height;

    // updates the screen:
    // polls GLFW events and handles window events
    bool update();

    // getters
    operator VkSurfaceKHR() const {return surface;}
    operator VkInstance() const {return instance;}
    operator GLFWwindow*() const {return window;}
};

};
#endif