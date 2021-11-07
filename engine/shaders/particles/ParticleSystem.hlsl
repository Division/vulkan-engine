#include "shaders/includes/global.hlsl"

struct Particle
{
	float4 position;
	float4 velocity;
};

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
	float dt;
	float lifetime_rate;
};

groupshared int final_particle_count;

[numthreads(64, 1, 1)]
void EmitParticlesDirection(uint3 id: SV_DispatchThreadID)
{
	final_particle_count = counters[COUNTER_INDEX_ALIVE];
	AllMemoryBarrierWithGroupSync();

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
			InterlockedAdd(final_particle_count, +1, alive_particle_count);
			alive_indices[alive_particle_count] = particle_index;

			particles[particle_index].position = float4(emit_position, 1);
			particles[particle_index].velocity = float4(emit_direction, 0);
		}
	}

	AllMemoryBarrierWithGroupSync();
	if (id.x == 0)
	{
		int num_dispatch_groups = (int)ceil(float(final_particle_count) / 64.0f);
		counters[COUNTER_INDEX_UPDATE_DISPATCH_GROUPS] = num_dispatch_groups;
		counters[COUNTER_INDEX_ALIVE] = final_particle_count;
	}

}

[numthreads(64, 1, 1)]
void UpdateParticles(uint3 id: SV_DispatchThreadID)
{
	final_particle_count = counters[COUNTER_INDEX_ALIVE];
	AllMemoryBarrierWithGroupSync();

	if (id.x < final_particle_count)
	{
		int particle_index = alive_indices[id.x];

		particles[particle_index].position.xyz += particles[particle_index].velocity.xyz * dt;
		particles[particle_index].velocity.w += lifetime_rate * dt;

		if (particles[particle_index].velocity.w < 1.0f)
		{
			int current_instance;
			InterlockedAdd(counters[COUNTER_INDEX_INSTANCE_COUNT], +1, current_instance);
			draw_indices[current_instance] = particle_index;
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
