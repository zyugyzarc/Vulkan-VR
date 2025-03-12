#ifndef ENTITY_CPP
#define ENTITY_CPP

#ifndef HEADER
    #define HEADER
    #include "../vk/vklib.h"
    #include "mesh.cpp"
    #include "material.cpp"
    #include "camera.cpp"
    #undef HEADER
#endif

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp> 

namespace sc {


struct uni_Transform_t {
    glm::mat4 model;
    glm::mat4 norm;
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec3 camerapos;
    float t;
};

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

    void set_transforms(vk::CommandBuffer&);
    void draw(vk::CommandBuffer&);
};

}; // end of instance.h file
#ifndef HEADER
namespace sc {

// initialize the entity with the mesh, material, and make buffers for the transforms
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

// sets descriptor (set=0, binding=0) to the model's transforms. must call descriptorSet(0) beforehand.
void Entity::set_transforms(vk::CommandBuffer& cmd) {
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
    tf->camerapos = camera.pos;
    tf->t += 1./60;
    
    // send it off to the shaders
    mat.writeDescriptor(0, 0, *transforms[tbuf_idx], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    // use the next buffer next time
    tbuf_idx = (tbuf_idx + 1) % 8;
}

// draw the current entity
void Entity::draw(vk::CommandBuffer& cmd) {

    // send it off to the shaders
    mat.descriptorSet(0);
    set_transforms(cmd);

    // now bind the pipeline
    mat.bind(cmd);

    // we're all set to draw the mesh now
    mesh.draw(cmd);
}

Entity::~Entity() {
    for (auto i : transforms) {
        delete i;
    }
}

};
#endif
#endif