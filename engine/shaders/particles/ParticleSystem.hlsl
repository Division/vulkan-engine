#include "shaders/includes/global.hlsl"
#include "shaders/includes/random.hlsl"
#include "shaders/includes/math_defines.hlsl"
#include "shaders/particles/particle_material_common.hlsl"

RWStructuredBuffer<Particle> particles : register(u4, space0);
RWStructuredBuffer<uint> dead_indices : register(u5, space0);
RWStructuredBuffer<uint> alive_indices : register(u6, space0);
RWStructuredBuffer<uint> draw_indices : register(u7, space0);
RWStructuredBuffer<int> counters : register(u8, space0);
Texture2D noise : register(t9, space0);

#define COUNTER_INDEX_ALIVE 8
#define COUNTER_INDEX_DEAD 9
#define COUNTER_INDEX_UPDATE_DISPATCH_GROUPS 0
#define COUNTER_INDEX_INSTANCE_COUNT 4

cbuffer GlobalConstants : register(b10, space0)
{
	uint emit_count;
	float3 emit_position;
	float3 emit_direction;

	float4 color;
	float3 emit_size_min;
	float3 emit_size_max;
	float angle;

	float time;
	float dt;
	float lifetime_rate;
};

groupshared int final_particle_count;

[numthreads(64, 1, 1)]
void EmitParticles(uint3 id: SV_DispatchThreadID, uint3 groupthread_id : SV_GroupThreadID)
{
	if (id.x < emit_count)
	{
		int available_particle_count;
		InterlockedAdd(counters[COUNTER_INDEX_DEAD], -1, available_particle_count);
		if (available_particle_count <= 0)
		{
			InterlockedAdd(counters[COUNTER_INDEX_DEAD], +1, available_particle_count);
		}
		else
		{
			uint particle_index = dead_indices[available_particle_count - 1];
			uint alive_particle_count;
			InterlockedAdd(counters[COUNTER_INDEX_ALIVE], +1, alive_particle_count);
			alive_indices[alive_particle_count] = particle_index;

			float random_angle = get_random_number(make_random_seed(uint2(random_quantize(time / 3600.0f), id.x))) * PI * 2.0f;

			particles[particle_index].position = float4(emit_position, 1);
			particles[particle_index].velocity = float4(cos(random_angle), 0, sin(random_angle), 0) * 5.0f;
			particles[particle_index].color = color;
			particles[particle_index].size = emit_size_min + (emit_size_max - emit_size_min) * get_random_number(make_random_seed(uint2(random_quantize(time / 7200.0f), id.x + 1000)));
		}
	}

	AllMemoryBarrierWithGroupSync();
	if (groupthread_id.x == 0)
	{
		counters[COUNTER_INDEX_INSTANCE_COUNT] = 0;
		int num_dispatch_groups = (int)ceil(float(counters[COUNTER_INDEX_ALIVE]) / 64.0f);
		InterlockedMax(counters[COUNTER_INDEX_UPDATE_DISPATCH_GROUPS], num_dispatch_groups);
	}

}

ParticleUpdateData GetParticleUpdateData()
{
	ParticleUpdateData data;

	data.particles = particles;
	data.color = color;
	data.size_min = emit_size_min;
	data.size_max = emit_size_max;
	data.time = time;
	data.lifetime_rate = lifetime_rate;

	return data;
}

[numthreads(64, 1, 1)]
void UpdateParticles(uint3 id: SV_DispatchThreadID)
{
	final_particle_count = counters[COUNTER_INDEX_ALIVE];
	AllMemoryBarrierWithGroupSync();

	if (id.x < final_particle_count)
	{
		int particle_index = alive_indices[id.x];

		ParticleUpdateData data = GetParticleUpdateData();
		ProcessParticle(data, dt, particle_index);

		if (particles[particle_index].velocity.w < 1.0f)
		{
			int current_instance;
			InterlockedAdd(counters[COUNTER_INDEX_INSTANCE_COUNT], +1, current_instance);
			draw_indices[current_instance] = particle_index;
			alive_indices[current_instance] = particle_index;
		}
		else
		{
			int dead_index;
			InterlockedAdd(counters[COUNTER_INDEX_DEAD], +1, dead_index);
			dead_indices[dead_index] = particle_index;

			InterlockedAdd(counters[COUNTER_INDEX_ALIVE], -1);
		}
	}
}
