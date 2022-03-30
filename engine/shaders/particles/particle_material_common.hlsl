#ifndef _PARTICLE_COMMON_
#define _PARTICLE_COMMON_

#include "shaders/material/material_common.hlsl"

struct Particle
{
	float4 position;
	float4 velocity;
	float4 color;
	float3 size;
};

struct ParticleUpdateData
{
	RWStructuredBuffer<Particle> particles;
	float4 color;
	float3 size_min;
	float3 size_max;
	float time;
	float lifetime_rate;
};

static const float3 PARTICLE_BILLBOARD[] =
{
	float3(1,  1,  0),
	float3(1, -1,  0),
	float3(-1, -1,  0),
	float3(-1,  1,  0)
};

VertexData GetDefaultParticleQuadVertexData(VS_in input)
{
    VertexData result;

    result.instance_id = input.instance_id;
    result.vertex_id = input.vertex_id;

    result.position_worldspace = float4(input.position.xyz, 1.0);
    result.origin = float4(0, 0, 0, 1);

#if defined(TEXTURE0) || defined(NORMAL_MAP)
    result.texcoord0 = input.texcoord0;
#endif

    result.instance_id = input.instance_id;
    result.frag_coord = 0;

    return result;
}

VS_out GetParticleQuadVSOut(VertexData data, Particle particle, float4x4 view_matrix, float4x4 projection_matrix)
{
    VS_out result;

    result.instance_id = data.instance_id;
#if defined(TEXTURE0) || defined(NORMAL_MAP)
    result.texcoord0 = data.texcoord0;
#endif

    float4 position_worldspace = float4(particle.position.xyz, 1);
    float4 position_viewspace = mul(view_matrix, position_worldspace) + float4(PARTICLE_BILLBOARD[data.vertex_id] * particle.size, 0);

#if !defined(DEPTH_ONLY)
    result.position_worldspace = position_worldspace;
#endif

    result.position = mul(projection_matrix, position_viewspace);

//#if defined(LIGHTING)
    //result.normal_worldspace = data.normal_worldspace;
    //result.tangent_worldspace = data.tangent_worldspace;
    //result.linear_depth = LinearizeDepth(result.position.z / result.position.w, camera.zMin, camera.zMax);
//#endif

#if defined(VERTEX_ORIGIN)
    result.origin = data.origin;
#endif

    return result;
}

#endif