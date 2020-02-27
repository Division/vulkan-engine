#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 1) in vec2 frag_texcoord;

layout(location = 0) out vec4 out_color;

void main() 
{
	out_color = vec4(frag_texcoord.x,frag_texcoord.y,1,1);
}