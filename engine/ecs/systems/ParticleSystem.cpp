#include "ParticleSystem.h"
#include "ecs/components/Particles.h"
#include "ecs/components/Static.h"
#include "ecs/components/Transform.h"
#include "render/device/VulkanRenderState.h"
#include "render/mesh/Mesh.h"
#include "render/device/VulkanUtils.h"
#include "render/buffer/GPUBuffer.h"
#include "render/device/VulkanUtils.h"
#include "render/effects/GPUParticles.h"

using namespace ECS::components;
using namespace render;

namespace ECS::systems
{
	GPUParticleUpdateSystem::GPUParticleUpdateSystem(EntityManager& manager)
		: System(manager)
	{
	}

	GPUParticleUpdateSystem::~GPUParticleUpdateSystem() = default;

	void GPUParticleUpdateSystem::Process(Chunk* chunk)
	{
		OPTICK_EVENT();
		ComponentFetcher<ParticleEmitter> particle_emitter_fetcher(*chunk);
		ComponentFetcher<Transform> transform_fetcher(*chunk);

		auto uploader = Engine::GetVulkanContext()->GetUploader();

		auto dt = manager.GetStaticComponent<components::DeltaTime>()->dt;

		for (int i = 0; i < chunk->GetEntityCount(); i++)
		{

			auto* particle_emitter = particle_emitter_fetcher.GetComponent(i);
			auto& gpu_data = particle_emitter->GetGPUData();
			if (!gpu_data.GetMesh() || !gpu_data.GetMaterial())
				continue;

			auto* transform = transform_fetcher.GetComponent(i);

			uint32_t emit_count = 0;
			if (particle_emitter->emission_enabled && particle_emitter->emission_params.emission_rate > 0)
			{
				particle_emitter->time_since_last_emit += dt;
				const float time_per_particle = 1.0f / (float)particle_emitter->emission_params.emission_rate;
				emit_count = floorf(particle_emitter->time_since_last_emit / time_per_particle);
				particle_emitter->time_since_last_emit -= emit_count * time_per_particle;
			}

			auto actual_emit_shader = gpu_data.GetEmitShader();

			Device::ResourceBindings resources;
			Device::ConstantBindings constants;
			const float time = Engine::Get()->time();

			constants.AddUIntBinding(&emit_count, "emit_count");
			constants.AddFloatBinding(&time, "time");
			constants.AddFloatBinding(&dt, "dt");
			particle_emitter->FlushEmitConstants(constants, &transform->position);
			gpu_data.FlushExtraConstants(constants);
			gpu_data.FlushExtraResources(resources);
			
			const uint32_t threads = std::max(ceilf(emit_count / 64.0f), 1.0f);
			state->Dispatch(*actual_emit_shader, resources, constants, uvec3(threads, 1, 1));
		}
	}

	void GPUParticleUpdateSystem::ProcessChunks(const ChunkList::List& list, const Device::ConstantBindings& global_constants, Device::VulkanRenderState& state)
	{
		this->global_constants = &global_constants;
		this->state = &state;

		ProcessChunks(list);

		this->global_constants = nullptr;
		this->state = nullptr;
	}

}