#version 450
layout (location = 0) in vec3 fnorm; layout (location = 1) in vec2 fuv; layout (location = 0) out vec4 col; void main() { float v = fnorm.z; col = vec4(fnorm, 1.0); }