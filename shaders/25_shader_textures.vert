#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(std140, set = 0, binding = 0) uniform Camera {
    vec3 cameraPosition;
    ivec2 cameraScreenSize;
    mat4 cameraViewMatrix;
    mat4 cameraProjectionMatrix;
} camera;

layout(std140, set = 0, binding = 1) uniform ObjectParams {
    mat4 objectModelMatrix;
    mat4 objectNormalMatrix;
    vec2 uvScale;
    vec2 uvOffset;
    uint layer;
} object_params;


layout(location = 0) in vec3 position;
//layout(location = 1) in vec3 color;
layout(location = 2) in vec2 texcoord0;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    gl_Position = camera.cameraProjectionMatrix * camera.cameraViewMatrix * object_params.objectModelMatrix * vec4(position, 1.0);
    fragColor = vec3(1.0, 1.0, 1.0);
    fragTexCoord = texcoord0;
}
