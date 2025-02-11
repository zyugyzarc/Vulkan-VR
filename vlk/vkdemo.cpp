#include "vklib.h"

#include <iostream>
#include <string>

std::string vert_shadercode = SHADERCODE(

    vec2 positions[3] = vec2[](
        vec2(0.0, -0.5),
        vec2(0.5, 0.5),
        vec2(-0.5, 0.5)
    );

    void main() {
        gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    }
);

std::string frag_shadercode = SHADERCODE(
    
    layout(location = 0) out vec4 outColor;

    void main() {
        outColor = vec4(1.0, 0.0, 0.0, 1.0);
    }
);

int main() {

    // create the instance and the device    
    vk::Instance& instance = *new vk::Instance();
    vk::Device& dev = *new vk::Device(instance);

    // create the required queues
    vk::Queue& graphics = dev.create_queue(VK_QUEUE_GRAPHICS_BIT);
    vk::Queue& presentation = dev.create_queue(VK_QUEUE_PRESENTATION_BIT);

    dev.init();  // initialize the device

    // create shader modules from above
    vk::ShaderModule& vert = *new vk::ShaderModule(dev, "_shader.vert", vert_shadercode);
    vk::ShaderModule& frag = *new vk::ShaderModule(dev, "_shader.frag", frag_shadercode);

    // create a graphics pipeline, with one color attachment, and the two shader modules
    vk::Pipeline& graphical = vk::Pipeline::Graphics(
        dev, 
        {}, {}, vert,
        {VK_FORMAT_B8G8R8A8_SRGB}, VK_FORMAT_UNDEFINED, frag
    );

    // idle loop
    for (int i = 0; instance.update() && i < 100000000; i++) {
        //
    }

    // cleanup
    delete &graphical;
    delete &vert;
    delete &frag;
    delete &dev;
    delete &instance;

    return 0;
}