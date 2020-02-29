#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform PushConstant {
    float exposure;
} push_constants;

layout(location = 1) in vec2 frag_texcoord;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform sampler2D src_texture;

vec4 ToneMap(vec4 src_color, float exposure)
{
	//return vec4(src_color.rgb / (src_color.rgb + vec3(1.0)), 1.0);
	return vec4(vec3(1.0) - exp(-src_color.rgb * exposure), 1.0);
}

void main()
{
	vec4 src_sample = texture(src_texture, frag_texcoord);
	out_color = ToneMap(src_sample, push_constants.exposure);
}