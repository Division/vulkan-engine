#include "shaders/particles/particle_material_common.hlsl"

void ProcessParticle(inout ParticleUpdateData data, float dt, int particle_index)
{
	Particle particle = data.particles[particle_index];
	
	particle.position.xyz += particle.velocity.xyz * dt;
	particle.velocity.w += data.lifetime_rate * dt;
	
	data.particles[particle_index] = particle;
}

#include "shaders/particles/ParticleSystem.hlsl"