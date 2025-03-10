#include "vk/vklib.h"
#include "sc/scenes.h"
#include "360util/webcam.h"

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
    layout (location = 1) out vec3 fpos;
    layout (location = 2) out vec2 fuv;

    void main() {
        gl_Position = tf.proj * tf.view * tf.model * vec4(pos, 1.)
                    ; // + vec4(0., sin(pos.x * 10 + tf.t * 30) * 0.1, 0., 0.);
        fnorm = (tf.norm * vec4(norm, 1.0)).xyz;
        fpos = pos;
        fuv = uv;
    }
);

int main() {

//----------------------------------------------//
//  Engine Initialization
//----------------------------------------------//

    // create the instance and the device    
    vk::Instance& instance = *new vk::Instance("Funny monke", (int)(2560*1.5), (int)(1440*1.5));
    vk::Device& dev = *new vk::Device(instance);

    // create the required queues
    vk::Queue& graphics = dev.create_queue(VK_QUEUE_GRAPHICS_BIT);
    vk::Queue& presentation = dev.create_queue(VK_QUEUE_PRESENTATION_BIT);
    vk::Queue& compute = dev.create_queue(VK_QUEUE_COMPUTE_BIT);
    vk::Queue& transfer = dev.create_queue(VK_QUEUE_TRANSFER_BIT);

    dev.init();  // initialize the device

    // synch structures
    VkSemaphore sem_img_avail = dev.semaphore();
    VkSemaphore sem_render_finish = dev.semaphore();
    VkSemaphore sem_post_finish = dev.semaphore();
    VkFence fence_wait_frame = dev.fence(true);

//----------------------------------------------//
//  Webcam init
//----------------------------------------------//

    cm::Webcam& probecam = *new cm::Webcam(dev);
    probecam.startStreaming();

//----------------------------------------------//
//  Roughness Init
//----------------------------------------------//

    vk::ShaderModule& roughblur_sh = *new vk::ShaderModule(dev, "roughblur.comp",
    SHADERCODE(
        layout (binding = 0, rgba8) uniform readonly image2D source;
        layout (binding = 1, rgba8) uniform           image2D halfway;
        layout (binding = 2, rgba8) uniform writeonly image2D dest;

        layout (push_constant) uniform config { int rad; } cfg;

        layout(local_size_x = 32, local_size_y = 32) in;

        void main() {

            const ivec2 maxres = ivec2(1024, 512);
            const int step = 1;
            
            ivec2 coord = ivec2(gl_GlobalInvocationID.xy); 
            vec4 color = vec4(0., 0., 0., 1.);
            float sc = abs(cfg.rad) * 2 + 1;
                  sc = float(step) / sc;

            float xstp = float(coord.y) / float(maxres.y);
                  xstp = (xstp - 0.5) * 2 * 3.14159;
                  xstp = cos(xstp);

            vec2 dir = cfg.rad > 0 ? vec2(xstp, 0.) : vec2(0., 1.);

            for (int i = -abs(cfg.rad); i <= abs(cfg.rad); i+=step) {
                vec2 jcoord = vec2(coord) + dir * i;
                ivec2 icoord;
                      icoord.x = int(mod(jcoord.x + maxres.x, maxres.x));
                      icoord.y = int(mod(jcoord.y + maxres.y, maxres.y));
                if (cfg.rad > 0) {
                    color.rgb += imageLoad(source, icoord).bgr * sc;
                } else {
                    color.rgb += imageLoad(halfway, icoord).bgr * sc;
                }
            }

            if (cfg.rad > 0) {
                imageStore(halfway, coord, color);
            } else {
                imageStore(dest, coord, color);
            }

        }
    )
    );

    vk::Pipeline& roughblur = vk::Pipeline::Compute(dev, {{
        {.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .stageFlags = VK_SHADER_STAGE_ALL},
        {.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .stageFlags = VK_SHADER_STAGE_ALL},
        {.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .stageFlags = VK_SHADER_STAGE_ALL}
    }}, {{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(int)}},
    roughblur_sh);

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
            
            float K_x1 = 0.; // const
            float K_x2 = 1.; // * x
            float K_x3 = 0.; // * x * y
            
            float K_y1 = 0.; // const
            float K_y2 = 0.; // * x
            float K_y3 = 1.; // * y
            float K_y4 = 0.23 / 1920.; // * x * x
            float K_y5 = -.03 / 1080.; // * y * y

            // distortion correction:
            if (coord.x < 1920 / 2) {
                // left image
                coord -= ivec2(1920/4, 1080/2); // pre transform
                
                vec2 cu = coord;
                
                // transform
                coord.x = int(K_x1 + K_x2 * cu.x + K_x3 *cu.x *cu.y);
                coord.y = int(K_y1 + K_y2 * cu.x + K_y3 *cu.y + K_y4*cu.x*cu.x + K_y5*cu.y*cu.y);

                coord +=  ivec2(1920/4, 1080/2); // post transform

                if (coord.x > 1920/2) {
                    coord.x = 2561;
                }
            }
            else {
                // right image
                coord -= ivec2(3*1920/4, 1080/2); // pre transform
                
                vec2 cu = coord;
                
                // transform
                coord.x = int(K_x1 + K_x2 * cu.x + K_x3 *cu.x *cu.y);
                coord.y = int(K_y1 + K_y2 * cu.x + K_y3 *cu.y + K_y4*cu.x*cu.x + K_y5*cu.y*cu.y);


                coord +=  ivec2(3*1920/4, 1080/2); // post transform

                if (coord.x < 1920/2) {
                    coord.x = -1;
                }
            }

            vec4 color = imageLoad(imagein, coord).bgra;  // for some reason the render output is in bgra

            color.rgb = linearToSRGB(color.rgb);

            imageStore(imageout, coord_final, color);
        }
    ));

    vk::Pipeline& postprocess = vk::Pipeline::Compute(dev, {{
        {.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .stageFlags = VK_SHADER_STAGE_ALL},
        {.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .stageFlags = VK_SHADER_STAGE_ALL}
    }}, {},
    postprocess_sh);

//----------------------------------------------//
//  Object/Entity Initialization
//----------------------------------------------//

    // initialize monke
    sc::Mesh& monke_mesh = *new sc::Mesh(dev, "suzane_smooth.obj");
    // sc::Mesh& monke_mesh = *new sc::Mesh(dev, "sphere.obj");

    sc::Material& monke_mat = *new sc::Material(dev, "default_mat", 
    _shader_vert_default,
    SHADERCODE(
        layout (set = 0, binding = 0) uniform Obj {  // transforms uniform
            mat4 model;
            mat4 norm;
            mat4 view;
            mat4 proj;
            vec3 camerapos;
            float t;
        } tf;
        layout (set = 0, binding = 1) uniform sampler2D probeimg;

        layout (location = 0) in vec3 fnorm;
        layout (location = 1) in vec3 fpos;
        layout (location = 2) in vec2 fuv;

        layout (location = 0) out vec4 col;

        float lerp (float v, float i0, float i1, float o0, float o1) {
            return o0 + (o1 - o0) * (v - i0) / (i1 - i0);
        }
        
        void main() {
            
            // given the camera direction, we need to find the light direction
            vec3 cameradir = normalize(tf.camerapos - fpos);
            vec3 dir = reflect(cameradir, fnorm);

            // use spherical coordinates
            // convert normal to a lattitude and longitude

            vec2 spcoord = vec2(
                lerp(atan(dir.z, dir.x), -3.1415, 3.1415, 0, 1),
                lerp(dir.y, -1, 1, 0, 1)
            );

            // rotate the spcoord a little
            spcoord.x = mod(spcoord.x + 0.25, 1.0);

            col = texture(probeimg, spcoord).bgra;
            col.a = 1.f;

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
        layout (location = 2) in vec2 fuv;

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
        .extent = {1920, 1080, 1},
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
        .extent = {1920, 1080, 1},
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
    },  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    draw_raw.view({
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = vk_COLOR_FORMAT,
        .subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT}
    });

    // roughblur image
    // get the camera image and blur (one axis) on it, store that in this image
    vk::Image& roughblur_im_half = *new vk::Image(dev, {
        .imageType = VK_IMAGE_TYPE_2D,
        .format = vk_COLOR_FORMAT,
        .extent = {1024, 512, 1}, // match probeimg format!
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    },  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    roughblur_im_half.view({
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = vk_COLOR_FORMAT,
        .subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT}
    });

    // roughblur image
    // get the camera image and blur it, store that in this image
    vk::Image& roughblur_im = *new vk::Image(dev, {
        .imageType = VK_IMAGE_TYPE_2D,
        .format = vk_COLOR_FORMAT,
        .extent = {1024, 512, 1}, // match probeimg format!
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    },  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    roughblur_im.view({
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = vk_COLOR_FORMAT,
        .subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT}
    });

    roughblur_im.sampler();

    // offscreen image
    // final image, blit this to the output swapchain image
    vk::Image& target_final = *new vk::Image(dev, {
        .imageType = VK_IMAGE_TYPE_2D,
        .format = vk_COLOR_FORMAT,
        .extent = {1920, 1080, 1},
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
    },  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    target_final.view({
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

        // network: wait for image
        vk::Image& probeimg = probecam.getNextImage();

        probeimg.view({
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = vk_COLOR_FORMAT,
            .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT}
        });

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

        // record the commandbuffer for blurring the camera image
        vk::CommandBuffer& roughblur_cmd = compute.command() << [&](vk::CommandBuffer& cmd) {

            // set the probeimg to be a storage-optimal (read)
            cmd.imageTransition(probeimg,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
                VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT
            );


            // set the middle to be read and write
            cmd.imageTransition(roughblur_im_half,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
                VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT
            );

            // set the roughblur to be storage-optimal (for write)
            cmd.imageTransition(roughblur_im,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
                VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT
            );

            // bind the images
            roughblur.descriptorSet(0);
            roughblur.writeDescriptor(0, 0, probeimg, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
            roughblur.writeDescriptor(0, 1, roughblur_im_half, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
            roughblur.writeDescriptor(0, 2, roughblur_im, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

            // bind the postprocessor
            cmd.bindPipeline(roughblur);

            // set the arg
            cmd.setPcr(roughblur, 0,  -1); // 10px Y-blur

            uint32_t groupCountX = (1920 + 31) / 32;  // Round up division
            uint32_t groupCountY = (1080 + 31) / 32;

            // apply the shader
            cmd.dispatch(groupCountX, groupCountY, 1);

            // set the arg
            cmd.setPcr(roughblur, 0,  1); // 10px X-blur
            
            // apply the shader again
            cmd.dispatch(groupCountX, groupCountY, 1);

        };

        // record the commandbuffer for drawing
        vk::CommandBuffer& draw_cmd = graphics.command() << [&](vk::CommandBuffer& cmd) {
            
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

            // add camera image as texture
            cmd.imageTransition(roughblur_im,
                // VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT
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
                {{0, 0}, {1920, 1080}}
            );

            // go form eye=-1 to eye=1
            for (int eye = -1; eye <= 1; eye += 2) {
                // set render area (L)
                sc::camera.eye = eye;
                cmd.setRenderArea(
                    {1920.f / 4 * (1+eye), 0.f, 1920.f / 2, 1080.f, 0.f, 1.f}, // viewport
                    {{1920 / 4 * (1+eye), 0}, {1920 / 2, 1080}} // scissor rect
                );

                // draw the monke
                // monke.draw(cmd);

                monke_mat.descriptorSet(0); // init descriptor set
                monke.set_transforms(cmd); // writes the transforms into monke_mat (set=0, binding=0)
                ((vk::Pipeline&)monke_mat).writeDescriptor(0, 1, roughblur_im, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

                monke_mat.bind(cmd); // bind the pipeline

                monke_mesh.draw(cmd);

                // draw the plane
                plane_001.draw(cmd);
            }

            // end rendering
            cmd.endRendering();

            // set the camera image back to normal
            cmd.imageTransition(roughblur_im,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
                VK_IMAGE_ASPECT_COLOR_BIT
            );

        };

        // record the commandbuffer for post-processing
        vk::CommandBuffer& postproc_cmd = compute.command() << [&](vk::CommandBuffer& cmd) {

            // set the draw-raw to be a storage-optimal
            cmd.imageTransition(draw_raw,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT
            );

            // now set the screen to be storage-optimal (for write)
            cmd.imageTransition(target_final,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
                VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT
            );

            // bind the images
            postprocess.descriptorSet(0);
            postprocess.writeDescriptor(0, 0, draw_raw, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
            // postprocess.writeDescriptor(0, 0, roughblur_im, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
            postprocess.writeDescriptor(0, 1, target_final, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

            // bind the postprocessor
            cmd.bindPipeline(postprocess);

            uint32_t groupCountX = (1920 + 31) / 32;  // Round up division
            uint32_t groupCountY = (1080 + 31) / 32;

            // apply the shader
            cmd.dispatch(groupCountX, groupCountY, 1);

        };

        // record the commandbuffer to blit offscreen rt
        vk::CommandBuffer& blit_cmd = transfer.command() << [&](vk::CommandBuffer& cmd) {

            // now set the target_final to be transfer src
            cmd.imageTransition(target_final,
                VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT
            );

            // set th screen to be transfer dst
            cmd.imageTransition(screen,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT
            );

            // blut the target onto the screen
            cmd.blit(
                target_final, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, {1920, 1080, 1},
                screen, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, {instance.width, instance.height, 1},
                VK_IMAGE_ASPECT_COLOR_BIT 
            );

            // turn the screen to present optimal
            cmd.imageTransition(screen,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
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

        compute.submit(roughblur_cmd, VK_NULL_HANDLE,
            {sem_img_avail}, {VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT}, {/* auto sync */});

        graphics.submit(draw_cmd, VK_NULL_HANDLE,
            {/*auto sync*/}, {/*auto sync*/}, {/* auto sync */});

        compute.submit(postproc_cmd, VK_NULL_HANDLE,
            {/*auto sync*/}, {/*auto sync*/}, {/* auto sync */});

        transfer.submit(blit_cmd, fence_wait_frame,
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

    probecam.stopStreaming();
    delete &probecam;

    delete &graphics;
    delete &presentation;
    delete &compute;
    delete &transfer;

    delete &dev;
    delete &instance;

    return 0;
}