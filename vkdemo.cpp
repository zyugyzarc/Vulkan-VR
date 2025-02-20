#include "vk/vklib.h"
#include "sc/scenes.h"

#include <iostream>
#include <string>
#include <chrono>

std::string  _shader_vert_default = SHADERCODE(
    layout (set = 0, binding = 0) uniform Obj {  // transforms uniform
        mat4 model;
        mat4 norm;
        mat4 view;
        mat4 proj;
        float t;
    } tf;

    layout (location = 0) in vec3 pos;
    layout (location = 1) in vec3 norm;
    layout (location = 2) in vec2 uv;

    layout (location = 0) out vec3 fnorm;
    layout (location = 1) out vec2 fuv;

    void main() {
        gl_Position = tf.proj * tf.view * tf.model * vec4(pos, 1.)
                    ; // + vec4(0., sin(pos.x * 10 + tf.t * 30) * 0.1, 0., 0.);
        fnorm = (tf.norm * vec4(norm, 1.0)).xyz;

        fuv = uv;
    }
);

int main() {

//----------------------------------------------//
//  Engine Initialization
//----------------------------------------------//

    // create the instance and the device    
    vk::Instance& instance = *new vk::Instance("Funny monke", 2560, 1440);
    vk::Device& dev = *new vk::Device(instance);

    // create the required queues
    vk::Queue& graphics = dev.create_queue(VK_QUEUE_GRAPHICS_BIT);
    vk::Queue& presentation = dev.create_queue(VK_QUEUE_PRESENTATION_BIT);
    vk::Queue& compute = dev.create_queue(VK_QUEUE_COMPUTE_BIT);

    dev.init();  // initialize the device

    // synch structures
    VkSemaphore sem_img_avail = dev.semaphore();
    VkSemaphore sem_render_finish = dev.semaphore();
    VkSemaphore sem_post_finish = dev.semaphore();
    VkFence fence_wait_frame = dev.fence(true);

//----------------------------------------------//
//  Postprocessing Init
//----------------------------------------------//

    vk::ShaderModule& postprocess_sh = *new vk::ShaderModule(dev, "postprocess.comp",
    SHADERCODE(
        layout (binding = 0, rgba8) uniform readonly image2D imagein;
        layout (binding = 1, rgba8) uniform writeonly image2D imageout;

        layout(local_size_x = 32, local_size_y = 32) in;

        vec3 linearToSRGB(vec3 color) {
            // chatgpt cooked with this oneliner
            return mix(pow(color, vec3(1.0 / 2.4)) * 1.055 - 0.055, color * 12.92, lessThan(color, vec3(0.0031308)));
        }

        void main() {
            ivec2 coord = ivec2(gl_GlobalInvocationID.xy); 
            ivec2 coord_final = coord;

            float K_1 = 5. / (2560.*1440.);
            float K_2 = 5. / (2560.*2560.*1440.*1440.);
            float r2;
            vec2 sc;

            // distortion correction:
            if (coord.x < 2560 / 2) {
                // left image
                coord -= ivec2(2560/4, 1440/2); // pre transform
                r2 = coord.x*coord.x + coord.y*coord.y; // r
                sc = vec2(1 + K_1 * r2 + K_2 * r2 * r2); // transform
                coord = ivec2(sc * vec2(coord));
                coord +=  ivec2(2560/4, 1440/2); // post transform

                if (coord.x > 2560/2) {
                    coord.x = 2561;
                }
            }
            else {
                // right image
                coord -= ivec2(3*2560/4, 1440/2); // pre transform
                r2 = coord.x*coord.x + coord.y*coord.y; // r
                sc = vec2(1 + K_1 * r2 + K_2 * r2 * r2); // transform
                coord = ivec2(sc * vec2(coord));
                coord +=  ivec2(3*2560/4, 1440/2); // post transform

                if (coord.x < 2560/2) {
                    coord.x = -1;
                }
            }

            vec4 color = imageLoad(imagein, coord).bgra;  // for some reason it needs to be in bgra
            
            // vec4 color = vec4(0.0);

            // vec4 color = vec4(mod(float(coord.x), 100.) / 100., mod(float(coord.y), 100.) / 100., 0., 1.);

            // const int kernsize = 2;
            // const int samples = 2;

            // for (int i = -kernsize; i < kernsize; i+=kernsize/samples) {
            //     for (int j = -kernsize; j < kernsize; j+=kernsize/samples) {
            //         vec4 c = imageLoad(imagein, coord + ivec2(i, j)).bgra;
            //         color += c / float(samples * samples * 4);
            //     }
            // }

            color.rgb = linearToSRGB(color.rgb);

            // color.rgb = color.bbb; // use only blue

            imageStore(imageout, coord_final, color);
        }
    ));

    vk::Pipeline& postprocess = vk::Pipeline::Compute(dev, {{
        {.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .stageFlags = VK_SHADER_STAGE_ALL},
        {.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .stageFlags = VK_SHADER_STAGE_ALL}
    }}, postprocess_sh);

//----------------------------------------------//
//  Object/Entity Initialization
//----------------------------------------------//

    // initialize monke
    sc::Mesh& monke_mesh = *new sc::Mesh(dev, "suzane.obj");

    sc::Material& monke_mat = *new sc::Material(dev, "default_mat", 
    _shader_vert_default,
    SHADERCODE(
        layout (set = 0, binding = 0) uniform Obj {  // transforms uniform
            mat4 model;
            mat4 norm;
            mat4 view;
            mat4 proj;
            float t;
        } tf;

        layout (location = 0) in vec3 fnorm;
        layout (location = 1) in vec2 fuv;

        layout (location = 0) out vec4 col;
        
        void main() {
            float v = fnorm.y; // ah yes, classic lighting
                               // basically directional light from above
            col = vec4(v, v*0.5, v*0.25, 1.0);

            // float displacement = sin(gl_FragCoord.x /100. + tf.t * 30) * 0.05;

            // vec3 displacedPosition = gl_FragCoord.xyz + fnorm * displacement;

            // gl_FragDepth = displacedPosition.z;
        }
    )
    );

    // new monke object
    sc::Entity& monke = *new sc::Entity(dev, monke_mesh, monke_mat);

    // initialize plane
    sc::Mesh& plane_mesh = *new sc::Mesh(dev, "plane.obj");

    sc::Material& checkerboard_mat = *new sc::Material(dev, "checkerboard_mat", 
    _shader_vert_default,
    SHADERCODE(
        layout (location = 0) in vec3 fnorm;
        layout (location = 1) in vec2 fuv;

        layout (location = 0) out vec4 col;
        
        void main() {
            float v = fnorm.y; // ah yes, classic lighting
                               // basically directional light from above
            // make it checkerboard
            const float sz = 0.1;
            if ((mod(fuv.x, sz) > sz*0.5) ^^ (mod(fuv.y, sz) > sz*0.5)) {
                v *= 0.25;
            }
            
            col = vec4(v*0.5, v, v, 1.0);
        }
    )
    );

    // new plane object
    sc::Entity& plane_001 = *new sc::Entity(dev, plane_mesh, checkerboard_mat);
    
//----------------------------------------------//
//  Scene Setup
//----------------------------------------------//

    // setup scene
    sc::camera.pos = glm::vec3(0, 1.5, -5);
    sc::camera.target = glm::vec3(0, 0, 0);
    sc::camera.fov = 45;

    monke.position = glm::vec3(0, 0, 0);
    monke.rotation = glm::angleAxis(3.1415f / 2.f, glm::vec3(-1., 0., 0.));
    monke.scaling = glm::vec3(1, 1, 1);

    plane_001.position = glm::vec3(0, -.5, 0);
    plane_001.rotation = glm::quat(1, 0, 0, 0);
    plane_001.scaling = glm::vec3(4, 4, 4);


//----------------------------------------------//
//  Image Initialization
//----------------------------------------------//

    // depth buffer
    // use as a render-target depth attachment
    vk::Image& depth_buffer = *new vk::Image(dev, {
        .imageType = VK_IMAGE_TYPE_2D,
        .format = vk_DEPTH_FORMAT,
        .extent = {2560, 1440, 1},
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
    },  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    depth_buffer.view({
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = vk_DEPTH_FORMAT,
        .subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT}
    });

    // offscreen image
    // use as draw's render target, and then postprocess with compute
    vk::Image& draw_raw = *new vk::Image(dev, {
        .imageType = VK_IMAGE_TYPE_2D,
        .format = vk_COLOR_FORMAT,
        .extent = {2560, 1440, 1},
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
    },  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    draw_raw.view({
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = vk_COLOR_FORMAT,
        .subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT}
    });


//----------------------------------------------//
//  Main Loop
//----------------------------------------------//

    float t = 0;

    while (instance.update()) {
        
auto start_time = std::chrono::high_resolution_clock::now();

        // cpu: wait for the thing to be done
        dev.wait(fence_wait_frame);

        // get an image from the screen -- blocks
        vk::Image& screen = dev.getSwapchainImage(VK_NULL_HANDLE, sem_img_avail);

auto wait_time = std::chrono::high_resolution_clock::now();

        screen.view({
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = vk_COLOR_FORMAT,
            .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT}
        });

        // other stuff, global updates

        // rotate the monke
        monke.rotation = 
            glm::angleAxis(3.1415f / 2.f, glm::vec3(-1., 0., 0.)) *
            glm::angleAxis(t, glm::vec3(0, 0, 1));
        

//----------------------------------------------//
//  Main - Draw
//----------------------------------------------//

        // record the commandbuffer for drawing
        graphics.command() << [&](vk::CommandBuffer& cmd) {
            
            // set the draw-raw to be a color-attachment optimal
            cmd.imageTransition(draw_raw,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT
            );

            // set the depth to be a depth-attachment optimal
            cmd.imageTransition(depth_buffer,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
                VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, 
                    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
                    , VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                VK_IMAGE_ASPECT_DEPTH_BIT
            );

            // set the render target
            cmd.beginRendering(
                { VkRenderingAttachmentInfo{
                  .imageView = (VkImageView) draw_raw,
                  .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                  .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                  .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                  .clearValue = VkClearColorValue{0.f, 0.f, .1f, 1.f},
                }}, VkRenderingAttachmentInfo{
                    .imageView = depth_buffer,
                    .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                    .clearValue = 1.f //VkClearDepthStencilValue{1.0f, 0u},
                },
                {{0, 0}, {2560, 1440}}
            );

            // go form eye=-1 to eye=1
            for (int eye = -1; eye <= 1; eye += 2) {
                // set render area (L)
                sc::camera.eye = eye;
                cmd.setRenderArea(
                    {2560.f / 4 * (1+eye), 0.f, 2560.f / 2, 1440.f, 0.f, 1.f}, // viewport
                    {{2560 / 4 * (1+eye), 0}, {2560 / 2, 1440}} // scissor rect
                );

                // draw the monke
                monke.draw(cmd);

                // draw the plane
                plane_001.draw(cmd);
            }

            // end rendering
            cmd.endRendering();

        };

        // TODO
        // record the commandbuffer for post-processing
        compute.command() << [&](vk::CommandBuffer& cmd) {

            // set the draw-raw to be a storage-optimal
            cmd.imageTransition(draw_raw,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT
            );

            // now set the screen to be storage-optimal (for write)
            cmd.imageTransition(screen,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
                VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT
            );


            // bind the images
            postprocess.descriptorSet(0);
            postprocess.writeDescriptor(0, 0, draw_raw, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
            postprocess.writeDescriptor(0, 1, screen, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

            // bind the postprocessor
            cmd.bindPipeline(postprocess);  

            uint32_t groupCountX = (2560 + 31) / 32;  // Round up division
            uint32_t groupCountY = (1440 + 31) / 32;

            // apply the shader
            cmd.dispatch(groupCountX, groupCountY, 1);

            // turn the screen to present optimal
            cmd.imageTransition(screen,
                VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
                VK_IMAGE_ASPECT_COLOR_BIT
            );
        };

//----------------------------------------------//
//  Loop - Submit
//----------------------------------------------//
        
        // graphics.submit(VK_NULL_HANDLE,
        //     {sem_img_avail}, {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
        //     {sem_render_finish});

        // compute.submit(fence_wait_frame,
        //     {sem_render_finish}, {VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT},
        //     {sem_post_finish});
        
        // if we assume that we're on an igpu and graphics and compute are on the same qf

        graphics.submit(VK_NULL_HANDLE,
            {sem_img_avail}, {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}, {/* auto sync */});

        compute.submit(fence_wait_frame,
            {/*auto sync*/}, {/*auto sync*/}, {sem_post_finish});
        
        // throw the image onto the screen
        presentation.present(screen, {sem_post_finish});

auto end_time = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        auto waitduration = std::chrono::duration_cast<std::chrono::microseconds>(wait_time - start_time);

        t += duration.count() / 1000000.0;

        printf(" frametime: %03.3f ms (idle %03.3f ms) fps: %03.1f  \r",
                duration.count() / 1000.0,
                waitduration.count() / 1000.0,
                1000000.0 / duration.count());
        fflush(stdout);
    }

    dev.idle();

    // cleanup
    delete &monke;
    delete &monke_mat;
    delete &monke_mesh;

    delete &plane_001;
    delete &checkerboard_mat;
    delete &plane_mesh;

    delete &depth_buffer;
    delete &draw_raw;

    delete &postprocess;
    delete &postprocess_sh;

    delete &graphics;
    delete &presentation;
    delete &compute;

    delete &dev;
    delete &instance;

    return 0;
}