#version 450 core
layout(location = 0) out vec4 fColor;

layout(set = 0, binding = 2) uniform sampler2D texture0;

layout(location = 0) in struct {
    vec4 Color;
    vec2 UV;
} In;

layout(push_constant) uniform uPushConstant {
    vec2 uScale;
    vec2 uTranslate;
} pc;

void main()
{
    fColor = In.Color * texture(texture0, In.UV.st);
}
