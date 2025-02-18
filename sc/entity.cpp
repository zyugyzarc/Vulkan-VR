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

// global constants
extern float aspect;  // aspect ratio
extern float eyedist;  // distance between the eyes (in world units)
extern float eyeshift;  // if -1, render for the left eye, if 1, for the right eye

struct Camera_t {
    glm::vec3 pos;
    glm::vec3 target;
    float fov;  // in degrees

    glm::mat4 view() const {
        return glm::translate(
            glm::lookAt(pos, target, glm::vec3(0.0f, 1.0f, 0.0f)),
            glm::vec3(eyedist * -eyeshift, 0, 0)
        );
    }

    glm::mat4 proj() const {
        glm::mat4 res = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.f);
        res[1][1] *= -1;
        return res;
    }
};

struct uni_Transform_t {
    glm::mat4 model;
    glm::mat4 norm;
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

    std::vector<vk::Buffer*> transforms;
    uint32_t tbuf_idx;

public:
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scaling;

    Entity(vk::Device& d, Mesh& mh, Material& mt);

    ~Entity();

    void draw(vk::CommandBuffer&);
};

}; // end of instance.h file
#ifndef HEADER
namespace sc {

Camera_t camera {};

float aspect = 16.f / 9;
float eyedist = 0;
float eyeshift = 0;

Entity::Entity(vk::Device& d, Mesh& mh, Material& mt): mesh(mh), mat(mt)  {

    for (int i = 0; i < 8; i++) {
        transforms.push_back(
            new vk::Buffer(d, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 
                sizeof(uni_Transform_t))
        );
    }
    tbuf_idx = 0;
}

// draw the current entity.
void Entity::draw(vk::CommandBuffer& cmd) {

    uni_Transform_t* tf = (uni_Transform_t*) transforms[tbuf_idx]->map();
    
    // set up the transforms
    tf->model = glm::translate(
                    glm::mat4_cast(rotation) *
                        glm::scale(glm::mat4(1.0), scaling),
                    position
                    );
    tf->norm = glm::transpose(glm::inverse(tf->model));
    tf->view = camera.view();
    tf->proj = camera.proj();
    
    // send it off to the shaders
    mat.descriptorSet(0);
    mat.writeDescriptor(0, 0, *transforms[tbuf_idx], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    // now bind the pipeline
    mat.bind(cmd);

    // we're all set to draw the mesh now
    mesh.draw(cmd);

    // use the next buffer next time
    tbuf_idx = (tbuf_idx + 1) % 8;
}

Entity::~Entity() {
    for (auto i : transforms) {
        delete i;
    }
}

};
#endif
#endif