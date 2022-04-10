#include "GPUParticles.h"
#include "utils/MeshGeneration.h"
#include "render/shader/ShaderCache.h"
#include "render/device/VulkanRenderState.h"
#include "render/renderer/RenderGraph.h"
#include "render/renderer/SceneRenderer.h"
#include "ecs/components/Particles.h"
#include "ecs/systems/ParticleSystem.h"
#include "ecs/ECS.h"

using namespace Device;
using namespace ECS;

namespace render::effects {

	GPUParticles::~GPUParticles() = default;
	
	GPUParticles::GPUParticles(SceneRenderer& scene_renderer, BitonicSort& bitonic_sort, ECS::EntityManager& manager)
		: scene_renderer(scene_renderer), manager(manager), bitonic_sort(bitonic_sort)
	{
		particle_update_system = std::make_unique<systems::GPUParticleUpdateSystem>(manager, scene_renderer, bitonic_sort);
	}

	void GPUParticles::Update(graph::RenderGraph& graph, const Device::ConstantBindings& global_constants, float dt)
	{
		auto emitters = manager.GetChunkListsWithComponent<components::ParticleEmitter>();
		particle_update_system->ProcessChunks(emitters, global_constants, graph);
	}

}