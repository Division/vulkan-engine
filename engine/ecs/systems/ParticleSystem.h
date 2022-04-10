#pragma once

#include "ecs/System.h"
#include "render/renderer/IRenderer.h"

namespace render::graph
{
	class RenderGraph;
}

namespace render
{
	class SceneRenderer;
	class DrawCallManager;
	class BitonicSort;
}

namespace Device
{
	class ShaderProgram;
	class VulkanRenderState;
	class ConstantBindings;
}

namespace ECS::systems
{
	class GPUParticleUpdateSystem : public System
	{
		render::SceneRenderer& scene_renderer;
		render::DrawCallManager& draw_call_manager;
		render::BitonicSort& bitonic_sort;
		render::graph::RenderGraph* graph = nullptr;
		const Device::ConstantBindings* global_constants = nullptr;
		Device::ShaderProgram* pre_sort_shader = nullptr;
		Device::ShaderProgram* pre_sort_args_shader = nullptr;
		Device::ShaderProgram* output_sorted_shader = nullptr;

		void ProcessChunks(const ChunkList::List& list) override
		{
			System::ProcessChunks(list);
		}

	public:
		GPUParticleUpdateSystem(EntityManager& manager, render::SceneRenderer& scene_renderer, render::BitonicSort& bitonic_sort);
		~GPUParticleUpdateSystem();

		void ProcessChunks(const ChunkList::List& list, const Device::ConstantBindings& global_constants, render::graph::RenderGraph& graph);

		void Process(Chunk* chunk) override;
	};

}