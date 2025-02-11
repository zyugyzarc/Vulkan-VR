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
        gl_Position = vec4(positions[gl_VertexIndex], 0.5, 1.0);
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

    VkSemaphore sem_img_avail = dev.semaphore();
    VkSemaphore sem_render_finish = dev.semaphore();
    VkFence fence_wait_frame = dev.fence();

    // idle loop
    for (int i = 0; instance.update() && i < 100000000; i++) {

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

        presentation.present(screen, {sem_render_finish});

        dev.wait(fence_wait_frame);

        printf("Frame!\n");
    }

    dev.idle();

    // cleanup
    delete &graphical;

    delete &vert;
    delete &frag;

    delete &graphics;
    
    delete &dev;
    delete &instance;

    return 0;
}