#ifndef CAMERA_CPP
#define CAMERA_CPP

#ifndef HEADER
    #define HEADER
    #include "../vk/vklib.h"
    #include "mesh.cpp"
    #include "material.cpp"
    #undef HEADER
#endif

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp> 


namespace sc {

class Camera {

public:

    // current params
    glm::vec3 pos;
    glm::vec3 target;
    float fov = 45;
    float near = 0.1;

    // state param
    int eye = 0; // must be -1, 0 or 1.

    // the functions
    glm::mat4 view() const;
    glm::mat4 proj() const;

};

// the global camera object used by the renderer
extern Camera camera;

}; // end of instance.h file
#ifndef HEADER
namespace sc {

Camera camera {};



// return a view matrix. shifts the image by `ipd`
// eye = -1 for left,
// eye = 0 for no effect, and
// eye =  1 for right
glm::mat4 Camera::view() const {


    float ipd = 64; // inter-pupilary distance (mm)

    return glm::translate(
        glm::lookAt(pos, target, glm::vec3(0.0f, 1.0f, 0.0f)),
        glm::vec3(0, 0, 0)
    );
}

// return a perspective projection matrix.
// eye = -1 for left,
// eye = 0 for no effect, and
// eye =  1 for right
glm::mat4 Camera::proj() const {

    if (eye == 0) {
        return glm::infinitePerspective(fov, 16.f/9.f, near);
    }

    float focal_length = 40;  // focal length of lens (mm)
    float ipd = 64; // inter-pupilary distance (mm)
    float screen_w = 120.96; // width of screen (mm)
    float screen_h = 68.04;  // height of screen (mm)

    float d_eye = 18; // distamce from lens to eye (mm)
    float d_screen = 39; // distance from lens to screen (mm)
    float d_image = 1 / (1/focal_length - 1/d_screen); // distance from lens to image (mm)

    float magnification = focal_length / (focal_length - d_screen); // magnification (factor)
    float znear = near; // near plane (word units)

    float image_h = magnification * screen_h; // height of image (mm)
    
    float top = znear/2 * image_h / (d_image + d_eye); // top of image (world units)
    float bottom = -top; // bottom of image (world units)
    
    float screen_w_1 = magnification * ipd/2; // width of image 1 (mm)
    float screen_w_2 = magnification * (screen_w - ipd)/2; // width of image 2 (mm)
    
    float l_left = znear * screen_w_1 / (d_image + d_eye); // left of image for left eye (world units)
    float l_right = -znear * screen_w_2 / (d_image + d_eye); // right of image for left eye (world units)

    float r_left = -l_left; // left of image for right eye (world units)
    float r_right = -l_right; // right of image for right eye (world units)
    
    float left, right;

    if (eye == 1) {
        left = l_left;
        right = l_right;
    }
    else {
        left = r_right;
        right = r_left;
    }

    glm::mat4 result(0.0f);

    result[0][0] = (2.0f * near) / (right - left);
    result[1][1] = (2.0f * near) / (top - bottom);
    result[2][0] = (right + left) / (right - left);
    result[2][1] = (top + bottom) / (top - bottom);
    result[2][2] = -1.0f;
    result[2][3] = -1.0f;
    result[3][2] = -2.0f * near;
    result[3][3] = 0.0f;

    return result;
}

};
#endif
#endif