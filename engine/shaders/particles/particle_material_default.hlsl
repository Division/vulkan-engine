#include "shaders/material/material_default.hlsl"
#include "shaders/particles/particle_material_common.hlsl"

VS_out vs_main(VS_in input, uint instance_id : SV_InstanceID)
{
	VertexData vertex_data = GetDefaultVertexData(input, objectModelMatrix, objectNormalMatrix);

	Particle particle = particles[draw_indices[instance_id]];
	vertex_data.position_worldspace.xyz += particle.position.xyz;

	return GetVSOut(vertex_data, mul(camera.cameraProjectionMatrix, camera.cameraViewMatrix));
}

float4 ps_main(VS_out input) : SV_TARGET
{
	return GetPSMaterialDefault(input);
}