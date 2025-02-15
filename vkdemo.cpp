#include "vk/vklib.h"

#include <iostream>
#include <string>

std::string vert_shadercode = SHADERCODE(
    
    layout (location = 0) in vec3 vertpos;
    layout (location = 1) in vec3 vertcol;

    layout (location = 0) out vec3 fragcol;

    void main() {
        gl_Position = vec4(vertpos, 1.0);
        fragcol = vertcol;
    }
);

std::string frag_shadercode = SHADERCODE(
    
    layout(location = 0) in vec3 vertcol;
    layout(location = 0) out vec4 outcol;

    void main() {
        outcol = vec4(vertcol, 1.0);
    }
);

struct Vertex {
    glm::vec3 pos;
    glm::vec3 col;
};

std::vector<Vertex> model = {
    {{0.f, -.5f, 0.f}, {1.f, 0.f, 0.f}},
    {{.5f, .5f, 0.f},  {0.f, 1.f, 0.f}},
    {{-.5f, .5f, 0.f}, {0.f, 0.f, 1.f}},
};

int main() {

    // create the instance and the device    
    vk::Instance& instance = *new vk::Instance("Funny Triangle", 2560, 1440);
    vk::Device& dev = *new vk::Device(instance);

    // create the required queues
    vk::Queue& graphics = dev.create_queue(VK_QUEUE_GRAPHICS_BIT);
    vk::Queue& presentation = dev.create_queue(VK_QUEUE_PRESENTATION_BIT);

    dev.init();  // initialize the device

    // create shader modules from above
    vk::ShaderModule& vert = *new vk::ShaderModule(dev, "_shader.vert", vert_shadercode);
    vk::ShaderModule& frag = *new vk::ShaderModule(dev, "_shader.frag", frag_shadercode);

    // create vert buffer
    vk::Buffer& vertbuffer = *new vk::Buffer(dev,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                sizeof(Vertex) * model.size());

    // copy the model onto the buffer
    vertbuffer.mapped([&](void* target) {
        
        char* src = (char*) model.data();
        char* dst = (char*) target;
        for (int i = 0; i < sizeof(Vertex) * model.size(); i++) {
            *dst = *src;
            dst++;
            src++;
        }

    });

    // create a graphics pipeline, with one color attachment, and the two shader modules
    vk::Pipeline& graphical = vk::Pipeline::Graphics(
        dev, {},
        {{.stride = sizeof(Vertex),
          .rate = VK_VERTEX_INPUT_RATE_VERTEX,
          .attr = {
            {.format=VK_FORMAT_R32G32B32_SFLOAT},
            {.format=VK_FORMAT_R32G32B32_SFLOAT, .offset=offsetof(Vertex, col)},
          }
          }}, vert,
        {VK_FORMAT_B8G8R8A8_SRGB}, VK_FORMAT_UNDEFINED, frag
    );

    VkSemaphore sem_img_avail = dev.semaphore();
    VkSemaphore sem_render_finish = dev.semaphore();
    VkFence fence_wait_frame = dev.fence();

    while (instance.update()) {

        // get an image from the screen
        vk::Image& screen = dev.getSwapchainImage(VK_NULL_HANDLE, sem_img_avail);

        screen.view({
            .image = screen,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = VK_FORMAT_B8G8R8A8_SRGB,
            .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT}
        });
        
        // record the commandbuffer
        graphics.command() << [&](vk::CommandBuffer& cmd) {
            
            // set the image to be a color-attachment optimal
            cmd.imageTransition(screen,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT
            );

            // set the render target
            cmd.beginRendering(
                { VkRenderingAttachmentInfo{
                  .imageView = screen,
                  .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                  .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                  .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                  .clearValue = VkClearColorValue{0.f, 0.f, .1f, 1.f}
                }}, {},
                {{0, 0}, {2560, 1440}}
            );

            // bind the graphics pipeline
            cmd.bindPipeline(graphical);

            cmd.bindVertexInput({&vertbuffer});

            // set render area
            cmd.setRenderArea(
                {0.f, 0.f, 2560.f, 1440.f, 0.f, 1.f}, // viewport
                {{0, 0}, {2560, 1440}} // scissor rect
            );

            // draw 3 verticies (triangle)
            cmd.draw(3, 1);

            // end rendering
            cmd.endRendering();

            // set the image to be a present-optimal
            cmd.imageTransition(screen,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
                VK_IMAGE_ASPECT_COLOR_BIT
            );

        };

        graphics.submit(fence_wait_frame,
            {sem_img_avail}, {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
            {sem_render_finish});
        
        // throw the image onto the screen
        presentation.present(screen, {sem_render_finish});

        // cpu: wait for the thing to be done
        dev.wait(fence_wait_frame);
    }

    dev.idle();

    // cleanup
    delete &graphical;

    delete &vert;
    delete &frag;

    delete &graphics;
    delete &presentation;

    delete &vertbuffer;
    
    delete &dev;
    delete &instance;

    return 0;
}