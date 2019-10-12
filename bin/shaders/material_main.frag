#version 450
#extension GL_ARB_separate_shader_objects : enable

#if defined(TEXTURE0)
layout(binding = 2) uniform sampler2D texture0;
layout(location = 1) in vec2 fragTexCoord;
#endif

layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 out_color;

void main() {
	out_color = vec4(1,1,1,1);
	
#if defined(TEXTURE0)
    vec4 texture0_color = texture(texture0, fragTexCoord);
	out_color *= texture0_color;
#endif
}