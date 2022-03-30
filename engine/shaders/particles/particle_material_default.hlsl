#include "shaders/material/material_default.hlsl"
#include "shaders/particles/particle_material_common.hlsl"

RWStructuredBuffer<Particle> particles : register(u6, space1);
RWStructuredBuffer<uint> draw_indices : register(u7, space1);

VS_out vs_main(VS_in input)
{
	VertexData vertex_data = GetDefaultParticleQuadVertexData(input);

	Particle particle = particles[draw_indices[vertex_data.instance_id]];
	return GetParticleQuadVSOut(vertex_data, particle, camera.cameraViewMatrix, camera.cameraProjectionMatrix);
}

float4 ps_main(VS_out input) : SV_TARGET
{
	VertexData vertex_data;
	float4 value = GetPSMaterialAndVertexDataDefault(input, vertex_data);

	Particle particle = particles[draw_indices[vertex_data.instance_id]];
	value.a *= saturate(1.0f - particle.velocity.w);

	return value;
}