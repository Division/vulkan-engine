#include "shaders/particles/particle_common.hlsl"

struct ParticleAttractor
{
	float3 position;
	float power;
};

RWStructuredBuffer<ParticleAttractor> attractors : register(u14, space0);

cbuffer AdditionalConstants : register(b15, space0)
{
	uint attractor_count;
}

void ProcessParticle(inout ParticleUpdateData data)
{
	Particle particle = data.particles[data.particle_index];

	particle.position.xyz += particle.velocity * data.dt;
	particle.life += data.dt;

	float3 acceleration = 0;

	for (uint i = 0; i < attractor_count; i++)
	{
		float3 direction = attractors[i].position - particle.position.xyz;
		float length = distance(attractors[i].position, particle.position.xyz);
		if (length > 0.001f)
			direction /= length;
		else
			direction = float3(1, 0, 0);

		acceleration += direction * attractors[i].power / (length * length);
	}
	
	particle.velocity += acceleration * data.dt;

	data.particles[data.particle_index] = particle;
}

#include "shaders/particles/ParticleSystem.hlsl"