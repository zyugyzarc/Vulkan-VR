#include "vk/vklib.h"
#include "sc/scenes.h"

#include <iostream>
#include <string>
#include <chrono>

int main() {

    // create the instance and the device    
    vk::Instance& instance = *new vk::Instance("Funny monke", 2560, 1440);
    vk::Device& dev = *new vk::Device(instance);

    // create the required queues
    vk::Queue& graphics = dev.create_queue(VK_QUEUE_GRAPHICS_BIT);
    vk::Queue& presentation = dev.create_queue(VK_QUEUE_PRESENTATION_BIT);

    dev.init();  // initialize the device

    // initialize monke
    sc::Mesh& monke_mesh = *new sc::Mesh(dev, "suzane.obj");

    sc::Material& monke_mat = *new sc::Material(dev, "default_mat", 
    SHADERCODE(
        layout (set = 0, binding = 0) uniform Obj {
            mat4 model;
            mat4 view;
            mat4 proj;
        };

        layout (location = 0) in vec3 pos;
        layout (location = 1) in vec3 norm;
        layout (location = 2) in vec2 uv;

        layout (location = 0) out vec3 fnorm;
        layout (location = 1) out vec2 fuv;

        void main() {
            
            gl_Position = proj * view * model * vec4(pos, 1.0);

            fnorm = norm;
            fuv = uv;
        }
    ),
    SHADERCODE(
        layout (location = 0) in vec3 fnorm;
        layout (location = 1) in vec2 fuv;

        layout (location = 0) out vec4 col;
        
        void main() {
            float v = fnorm.z;
            col = vec4(v, v, v, v > 0.0 ? 1.0 : 0.0);
        }
    )
    );

    // new monke object
    sc::Entity& monke = *new sc::Entity(dev, monke_mesh, monke_mat);
    
    // setup scene
    sc::camera.pos = glm::vec3(0, 0, -5);
    sc::camera.target = glm::vec3(0, 0, 0);
    sc::camera.fov = 45;

    monke.position = glm::vec3(0, 0, 0);
    monke.rotation = glm::quat(1, 0, 0, 0);
    monke.scaling = glm::vec3(1, 1, 1);

    // synch structures
    VkSemaphore sem_img_avail = dev.semaphore();
    VkSemaphore sem_render_finish = dev.semaphore();
    VkFence fence_wait_frame = dev.fence(true);

    float t = 0;

    while (instance.update()) {
        
        // cpu: wait for the thing to be done
        dev.wait(fence_wait_frame);

        // get an image from the screen -- blocks
        vk::Image& screen = dev.getSwapchainImage(VK_NULL_HANDLE, sem_img_avail);

auto start_time = std::chrono::high_resolution_clock::now();

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

            // set render area
            cmd.setRenderArea(
                {0.f, 0.f, 2560.f, 1440.f, 0.f, 1.f}, // viewport
                {{0, 0}, {2560, 1440}} // scissor rect
            );

            // draw the monke
            monke.draw(cmd);

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

auto end_time = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

        printf(" frametime: %3.3f ms  fps: %3.1f  \r", duration.count() / 1000.0, 1000000.0 / duration.count());
        fflush(stdout);
    }

    dev.idle();

    // cleanup
    delete &monke;
    delete &monke_mat;
    delete &monke_mesh;

    delete &graphics;
    delete &presentation;

    delete &dev;
    delete &instance;

    return 0;
}