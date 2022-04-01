#include "shaders/particles/particle_common.hlsl"

void ProcessParticle(inout ParticleUpdateData data)
{
	Particle particle = data.particles[data.particle_index];

	particle.position.xyz += particle.velocity.xyz * data.dt;// +float3(0, 1, 0) * dt;
	particle.life += data.dt;

	data.particles[data.particle_index] = particle;
}

#include "shaders/particles/ParticleSystem.hlsl"