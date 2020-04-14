#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(std140, set = 0, binding = 0) uniform Camera {
    vec3 cameraPosition;
    ivec2 cameraScreenSize;
    mat4 cameraViewMatrix;
    mat4 cameraProjectionMatrix;
} camera;

layout(location = 0) in vec3 position;
layout(location = 1) out vec3 texcoord;

void main()
{
    vec3 localPos = aPos;
    texcoord = aPos;
    mat4 rotView = mat4(mat3(camera.cameraViewMatrix)); // remove translation from the view matrix
    vec4 clipPos = camera.cameraProjectionMatrix * rotView * vec4(localPos, 1.0);

    gl_Position = clipPos.xyww;
    /*texcoord = position.xyz;
    vec3 position_cameraspace = mat3(camera.cameraViewMatrix) * position;
    gl_Position = normalize(camera.cameraProjectionMatrix * vec4(position_cameraspace, 1));*/
}