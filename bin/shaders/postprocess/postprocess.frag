#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 1) in vec2 frag_texcoord;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform sampler2D src_texture;

void main()
{
	vec4 src_sample = texture(src_texture, frag_texcoord);

	float luminance = dot(src_sample, src_sample);

	out_color = vec4(luminance, luminance, luminance, 1);
}