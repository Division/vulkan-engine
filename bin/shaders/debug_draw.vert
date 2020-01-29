#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(std140, set = 0, binding = 0) uniform Camera {
    vec3 cameraPosition;
    ivec2 cameraScreenSize;
    mat4 cameraViewMatrix;
    mat4 cameraProjectionMatrix;
} camera;

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) out vec4 vertex_color;

void main() {
    vertex_color = color;
    vec4 position_worldspace = vec4(position, 1.0);
    gl_Position = camera.cameraProjectionMatrix * camera.cameraViewMatrix * position_worldspace;
    gl_PointSize = color.w;
}
