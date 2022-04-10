#include "shaders/includes/global.hlsl"
#include "shaders/includes/random.hlsl"
#include "shaders/includes/math_defines.hlsl"
#include "shaders/particles/particle_common.hlsl"

RWStructuredBuffer<Particle> particles : register(u4, space0);
RWStructuredBuffer<uint> dead_indices : register(u5, space0);
RWStructuredBuffer<uint> alive_indices : register(u6, space0);
RWStructuredBuffer<uint> draw_indices : register(u7, space0);
RWStructuredBuffer<int> counters : register(u8, space0);

cbuffer GlobalConstants : register(b10, space0)
{
	float time;
	float dt;

	// Emit
	uint emitter_id;
	uint emit_count;
	float3 emit_position;
	float3 emit_offset; // e.g. for line emitter
	float  emit_radius;
	float3 emit_direction;
	float2 emit_cone_angle;
	float2 emit_size;
	float2 emit_speed;
	float2 emit_life;
	float2 emit_angle;
	float4 emit_color;
};


#ifndef EMIT_FUNCTION
	#define EMIT_FUNCTION EmitParticleDefault
#endif

#ifndef PROCESS_FUNCTION
	#define PROCESS_FUNCTION ProcessParticle
#endif

Particle EmitParticleDefault(ParticleEmitData data)
{
	Particle particle;

	//float random_angle = get_random_number(make_random_seed(uint2(random_quantize(data.time / 3600.0f), data.particle_index + data.emitter_id))) * PI * 2.0f;
	particle.position = float4(GetParticleRandomPosition(data), 1);
	particle.velocity.xyz = GetParticleRandomDirection(data);
	particle.color = data.emit_color;
	particle.life = 0;
	particle.max_life = data.emit_life.x + (data.emit_life.y - data.emit_life.x) * get_random_number(make_random_seed(uint2(random_quantize(data.time / 8400.0f), data.particle_index + data.emitter_id + 2000)));
	particle.size.xyz = data.emit_size.x + (data.emit_size.y - data.emit_size.x) * get_random_number(make_random_seed(uint2(random_quantize(data.time / 7200.0f), data.particle_index + data.emitter_id + 1000)));

	return particle;
}
 
ParticleEmitData GetParticleEmitData(uint particle_index)
{
	ParticleEmitData data;

	data.particles = particles;
	data.emitter_id = emitter_id;
	data.particle_index = particle_index;
	data.time = time;
	data.emit_position = emit_position;
	data.emit_offset = emit_offset;
	data.emit_radius = emit_radius;
	data.emit_direction = emit_direction;
	data.emit_cone_angle = emit_cone_angle;
	data.emit_size = emit_size;
	data.emit_speed = emit_speed;
	data.emit_life = emit_life;
	data.emit_angle = emit_angle;
	data.emit_color = emit_color;

	return data;
}

ParticleUpdateData GetParticleUpdateData(uint particle_index)
{
	ParticleUpdateData data;

	data.particles = particles;
	data.particle_index = particle_index;
	data.time = time;
	data.dt = dt;

	return data;
}

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

			particles[particle_index] = EMIT_FUNCTION(GetParticleEmitData(particle_index));

			ParticleUpdateData data = GetParticleUpdateData(particle_index);
			data.dt = dt * (float)id.x / (float)emit_count;
			PROCESS_FUNCTION(data);
		}
	}

	AllMemoryBarrierWithGroupSync();
	if (groupthread_id.x == 0)
	{
		counters[COUNTER_INDEX_INSTANCE_COUNT] = 0;
		counters[COUNTER_INDEX_ALIVE_AFTER_SIMULATION] = 0;
		int num_dispatch_groups = (int)ceil(float(counters[COUNTER_INDEX_ALIVE]) / 64.0f);
		InterlockedMax(counters[COUNTER_INDEX_UPDATE_DISPATCH_GROUPS], num_dispatch_groups);
	}

}

groupshared int final_particle_count;

[numthreads(64, 1, 1)]
void UpdateParticles(uint3 id: SV_DispatchThreadID)
{
	if (id.x < counters[COUNTER_INDEX_ALIVE])
	{
		int particle_index = alive_indices[id.x];

		ParticleUpdateData data = GetParticleUpdateData(particle_index);
		PROCESS_FUNCTION(data);

		if (particles[particle_index].life < particles[particle_index].max_life)
		{
			int current_instance;
			InterlockedAdd(counters[COUNTER_INDEX_INSTANCE_COUNT], +1, current_instance);
			InterlockedAdd(counters[COUNTER_INDEX_ALIVE_AFTER_SIMULATION], +1);
			draw_indices[current_instance] = particle_index;
		}
		else
		{
			int dead_index;
			InterlockedAdd(counters[COUNTER_INDEX_DEAD], +1, dead_index);
			dead_indices[dead_index] = particle_index;
		}
	}
}
