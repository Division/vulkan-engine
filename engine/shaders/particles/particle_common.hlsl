#ifndef _PARTICLE_COMMON_
#define _PARTICLE_COMMON_

#include "shaders/material/material_common.hlsl"
#include "shaders/includes/random.hlsl"

struct Particle
{
	float4 position;
	float4 color;
	float3 velocity;
    float life;
	float3 size;
    float max_life;
};

struct ParticleEmitData
{
    RWStructuredBuffer<Particle> particles;
    float time;
    uint emitter_id;
    uint particle_index;
    float3 emit_position;
    float3 emit_offset;
    float  emit_radius;
    float3 emit_direction;
    float2 emit_cone_angle;
    float2 emit_size;
    float2 emit_speed;
    float2 emit_life;
    float2 emit_angle;
    float4 emit_color;
};

struct ParticleUpdateData
{
	RWStructuredBuffer<Particle> particles;
    uint particle_index;
	float time;
	float dt;
};

static const float3 PARTICLE_BILLBOARD[] =
{
	float3(1,  1,  0),
	float3(1, -1,  0),
	float3(-1, -1,  0),
	float3(-1,  1,  0)
};

float3 GetParticleRandomPosition(ParticleEmitData data)
{
    float3 float_seed3 = data.time * 0.0001f * data.emit_position;
    uint uint_seed = make_random_seed(uint2(data.emitter_id, data.particle_index));

#if defined(EMITTER_GEOMETRY_LINE)
    return random_point_on_line(data.emit_position, data.emit_position + data.emit_offset, make_random_seed(random_quantize3(float_seed3)) + uint_seed);
#elif defined(EMITTER_GEOMETRY_SPHERE)
    return random_point_on_sphere(data.emit_position, data.emit_radius, random_quantize3(float_seed3) + uint_seed);
#else
    return data.emit_position;
#endif
}

float3 GetParticleRandomDirection(ParticleEmitData data)
{
    float3 float_seed3 = data.time * 0.00215f * data.emit_position;
    //uint2 seed = uint2(make_random_seed(random_quantize3(float_seed3)), make_random_seed(uint2(data.emitter_id, data.particle_index)));
    uint2 seed = uint2(make_random_seed(random_quantize3(float_seed3)), data.particle_index);
    float3 direction = random_direction_in_cone(data.emit_direction, data.emit_cone_angle.x, seed);
    float speed = data.emit_speed.x + (data.emit_speed.y - data.emit_speed.x) * get_random_number(make_random_seed(seed));
    return direction * speed;
}

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
    float4 position_viewspace = mul(view_matrix, position_worldspace) + float4(PARTICLE_BILLBOARD[data.vertex_id] * particle.size.xyz, 0);

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