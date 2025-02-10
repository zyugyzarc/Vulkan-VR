
# Lighting, Occusion and Shadows of virtual objects in mixed reality

## Overall:

- Lighting
    - Fetch an environment texture from the 360 camera
    - Use the environment texture to render lighting on the virtual objects
- Occlusion
    - Use the environment texture and IMU to extract a user-perspective image from the camera
    - use depth estimation models to get a depth map
    - rescale the depthmap using ultrasonic sensor measurement
    - render the image and depth onto the virtual scene
- Shadows
    - Unproject the depth map from the previous step
    - apply a threshold filter on the environment to get emissive objects
    - draw a shadow from each emmisive object to the unprojected environment

# Todo-list

- Get Vulkan to render a triangle in stereo
- Make a simple obj parser/loader
- figure out a way to get opencv camera image to a vulkan texture
- figure out a way to get midas (midas cpp?) to pass a depth image into a vulkan texture