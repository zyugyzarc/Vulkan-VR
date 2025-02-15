#version 450
layout(location = 0) in vec3 vertcol; layout(location = 0) out vec4 outcol; void main() { outcol = vec4(vertcol, 1.0); }