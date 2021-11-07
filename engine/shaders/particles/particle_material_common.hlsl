#include "shaders/material/material_common.hlsl"

struct Particle
{
	float4 position;
	float4 velocity;
};

RWStructuredBuffer<Particle> particles : register(u6, space1);
RWStructuredBuffer<uint> draw_indices : register(u7, space1);
