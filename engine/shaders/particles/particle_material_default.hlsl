#include "shaders/material/material_default.hlsl"
#include "shaders/particles/particle_material_common.hlsl"

VS_out vs_main(VS_in input)
{
	VertexData vertex_data = GetDefaultVertexData(input, objectModelMatrix, objectNormalMatrix);

	Particle particle = particles[draw_indices[vertex_data.instance_id]];
	vertex_data.position_worldspace.xyz += particle.position.xyz;

	return GetVSOut(vertex_data, mul(camera.cameraProjectionMatrix, camera.cameraViewMatrix));
}

float4 ps_main(VS_out input) : SV_TARGET
{
	VertexData vertex_data;
	float4 value = GetPSMaterialAndVertexDataDefault(input, vertex_data);

	Particle particle = particles[draw_indices[vertex_data.instance_id]];
	value.a = saturate(1.0f - particle.velocity.w);

	return value;
}