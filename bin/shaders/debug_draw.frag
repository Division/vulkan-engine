#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 2) in vec4 vertex_color;
layout(location = 0) out vec4 out_color;

void main() {
	out_color = vertex_color;
}