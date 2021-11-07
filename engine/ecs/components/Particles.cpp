#include "Particles.h"

namespace ECS::components
{
	namespace
	{
#pragma pack(push, 1)
		struct ParticleData
		{
			vec4 position;
			vec4 velocity;
		};
#pragma pack(pop)

		std::vector<uint32_t> GetSequentialIndices(uint32_t count)
		{
			std::vector<uint32_t> result(count);
			for (uint32_t i = 0; i < count; i++)
				result[i] = i;

			return result;
		}
	}

	ParticleEmitter::GPUData::GPUData(uint32_t particle_count)
		: particles("particles data", particle_count * sizeof(ParticleData), Device::BufferType::Storage)
		, dead_indices("particles dead indices", particle_count * sizeof(uint32_t), Device::BufferType::Storage, GetSequentialIndices(particle_count).data())
		, alive_indices0("particles alive indices 0", particle_count * sizeof(uint32_t), Device::BufferType::Storage)
		, alive_indices1("particles alive indices 1", particle_count * sizeof(uint32_t), Device::BufferType::Storage)
		, counters("particles counters", sizeof(CounterArgumentsData), Device::BufferType::Indirect, false, &CounterArgumentsData(particle_count, 6))
	{
	}

	void ParticleEmitter::Initialize(EntityManager& manager, EntityID id, ParticleEmitter* emitter)
	{
		emitter->gpu_data = std::make_unique<GPUData>(emitter->max_particles);
	}

}