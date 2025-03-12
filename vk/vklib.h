#ifndef VKLIB_H
#define VKLIB_H

#ifdef VKLIB_NO_GLFW
    #include <vulkan/vulkan.h>
#else
    #define GLFW_INCLUDE_VULKAN
    #include <GLFW/glfw3.h>
    #include <glm/glm.hpp>
#endif

#include <vector>
#include <string>
#include <bit>

#include "vkassert.h"

#include "instance.h"
#include "device.h"
#include "queue.h"
#include "shadermodule.h"
#include "image.h"
#include "commandbuffer.h"
#include "buffer.h"
#include "pipeline.h"
#include "renderpass.h"

#endif