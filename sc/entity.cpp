#ifndef ENTITY_CPP
#define ENTITY_CPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp> 

#ifndef HEADER
    #define HEADER
    #include "../vk/vklib.h"
    #include "mesh.cpp"
    #include "material.cpp"
    #undef HEADER
#endif


namespace sc {

struct Camera_t {
    glm::vec3 pos;
    glm::vec3 target;
    float fov;  // in degrees

    glm::mat4 view() const {
        return glm::lookAt(pos, target, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glm::mat4 proj(float aspect) const {
        glm::mat4 res = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.f);
        res[1][1] *= -1;
        return res;
    }
};

struct uni_Transform_t {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

// this is the global camera
extern Camera_t camera;

// represents an entity in the scene.
// contains a Mesh, a Material, and transforms
class Entity {

    Mesh& mesh;
    Material& mat;

    vk::Buffer& transforms;

public:
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scaling;

    Entity(vk::Device& d, Mesh& mh, Material& mt):
         transforms(*new vk::Buffer(d, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 
                sizeof(uni_Transform_t))),
         mesh(mh), 
         mat(mt) {};

    ~Entity() {delete &transforms;}

    void draw(vk::CommandBuffer&);
};

}; // end of instance.h file
#ifndef HEADER
namespace sc {

Camera_t camera {
    .pos = glm::vec3(0, 0, -5)
};

// draw the current entity.
void Entity::draw(vk::CommandBuffer& cmd) {

    uni_Transform_t* tf = (uni_Transform_t*) transforms.map();
    
    // set up the transforms
    glm::mat4 view = camera.view();
    glm::mat4 proj = camera.proj(16./9.);
    
    glm::mat4 model = glm::mat4(1.0);
              model = glm::scale(model, scaling);
              model = model * glm::mat4_cast(rotation);
              model = glm::translate(model, position);

    tf->view = view;
    tf->proj = proj;
    tf->model = model;
    
    // send it off to the shaders
    mat.descriptorSet(0);
    mat.writeDescriptor(0, 0, transforms, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    // now bind the pipeline
    mat.bind(cmd);

    // we're all set to draw the mesh now
    mesh.draw(cmd);
}

};
#endif
#endif