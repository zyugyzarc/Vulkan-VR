#ifndef ENTITY_CPP
#define ENTITY_CPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#ifndef HEADER
    #define HEADER
    #include "../vk/vklib.h"
    #include "mesh.cpp"
    #undef HEADER
#endif

namespace sc {

// represents an entity in the scene.
// contains a Mesh, a Material, and transforms
class Entity {

};

}; // end of instance.h file
#ifndef HEADER
namespace sc {



};
#endif
#endif