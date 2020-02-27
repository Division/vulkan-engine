#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 position;
layout(location = 3) in vec2 texcoord0;
layout(location = 1) out vec2 frag_texcoord;

void main()
{
    frag_texcoord = texcoord0;
    gl_Position = vec4(position, 1.0f);
}