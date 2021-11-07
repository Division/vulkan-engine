#pragma once

#include <stdint.h>
#include "render/Buffer/GPUBuffer.h"
#include "render/Buffer/DynamicBuffer.h"
#include "ecs/ECS.h"
#include <magic_enum/magic_enum.hpp>
#include "resources/MaterialResource.h"
#include "render/renderer/DrawCallManager.h"

namespace Device
{
	class ShaderProgram;
}

namespace render::graph
{
	struct ResourceWrapper;
	struct DependencyNode;
}

namespace ECS::systems
{
	class GPUParticleUpdateSystem;
}

class Mesh;

namespace ECS::components
{
	struct ParticleEmitter
	{
	private:
		static inline std::atomic_uint32_t instance_count;

	public:
		static uint32_t GetCount() { return instance_count.load(); }

		friend class systems::GPUParticleUpdateSystem;

		ParticleEmitter()
		{
			instance_count++;
		}

		~ParticleEmitter()
		{
			instance_count--;
		}

		ParticleEmitter(ParticleEmitter&&) = default;

#pragma pack(push, 1)
		struct CounterArgumentsData
		{
			/// DispatchIndirect args for Update()
			int32_t groupsX = 0;
			int32_t groupsY = 1;
			int32_t groupsZ = 1;
			//////////////////////////////////////

			/// DrawIndexedIndirect args for Render()
			int32_t indexCount = 6;
			int32_t instanceCount = 0;
			int32_t firstIndex = 0;
			int32_t vertexOffset = 0;
			int32_t firstInstance = 0;
			/////////////////////////////////////////

			// Persistent counters
			int32_t alive_count = 0; 
			int32_t dead_count = 0;
			//////////////////////

			CounterArgumentsData(int32_t capacity, int32_t index_count)
			{
				dead_count = capacity;
				indexCount = index_count;
			}
		};
#pragma pack(pop)

		struct GPUData
		{
			Device::GPUBuffer particles;
			Device::GPUBuffer dead_indices;
			Device::GPUBuffer alive_indices0;
			Device::GPUBuffer alive_indices1;
			Device::DynamicBuffer<uint8_t> counters;
			uint32_t pass_counter = 1;

			Device::GPUBuffer& GetAliveIndices() { return pass_counter % 2 == 0 ? alive_indices0 : alive_indices1; }
			Device::GPUBuffer& GetDrawIndices() { return pass_counter % 2 == 0 ? alive_indices1 : alive_indices0; }

			enum PassType : int
			{
				PassTypeEmit = 0,
				PassTypeUpdate,
				PassTypeRender
			};

			GPUData(uint32_t particle_count);
		};

		render::DrawCallManager::Handle draw_calls;
		render::MaterialUnion material;
		Common::Handle<Mesh> mesh;

		std::unique_ptr<GPUData> gpu_data;
		uint32_t max_particles = 10000;
		uint32_t emission_rate = 1000;
		bool emission_enabled = true;

		static void Initialize(EntityManager& manager, EntityID id, ParticleEmitter* emitter);

	private:
		Material::Handle actual_material; // Material copy with added particle buffers

	};
}