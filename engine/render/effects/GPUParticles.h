#pragma once

#include <memory>

namespace Device
{
	class Texture;
	class VulkanRenderState;
	class ShaderProgram;
	class ShaderCache;
	class ConstantBindings;
}

namespace ECS
{
	class EntityManager;
}

namespace ECS::systems
{
	class GPUParticleUpdateSystem;
}

namespace render::graph
{
	class RenderGraph;
	class IRenderPassBuilder;
}

namespace render
{
	class SceneRenderer;
	class DrawCallManager;
	class BitonicSort;
}

namespace render::effects {


	class GPUParticles
	{
	public:
		GPUParticles(SceneRenderer& scene_renderer, BitonicSort& bitonic_sort, ECS::EntityManager& manager);
		~GPUParticles();

		void Update(graph::RenderGraph& graph, const Device::ConstantBindings& global_constants, float dt);

	private:
		std::unique_ptr<ECS::systems::GPUParticleUpdateSystem> particle_update_system;
		SceneRenderer& scene_renderer;
		BitonicSort& bitonic_sort;
		ECS::EntityManager& manager;
	};

}