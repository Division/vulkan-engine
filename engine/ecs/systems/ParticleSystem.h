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
}

namespace Device
{
	class ShaderProgram;
	class VulkanRenderState;
}

namespace ECS::systems
{
	class GPUParticleUpdateSystem : public System
	{
		render::graph::RenderGraph& graph;
		Device::ShaderProgram& emit_shader;
		Device::ShaderProgram& update_shader;
		render::SceneRenderer& scene_renderer;
		render::DrawCallManager& draw_call_manager;

	public:
		GPUParticleUpdateSystem(
			EntityManager& manager,
			render::SceneRenderer& scene_renderer,
			render::DrawCallManager& draw_call_manager,
			render::graph::RenderGraph& graph,
			Device::ShaderProgram& emit_shader,
			Device::ShaderProgram& update_shader
		)
			: System(manager), graph(graph), emit_shader(emit_shader), update_shader(update_shader), scene_renderer(scene_renderer), draw_call_manager(draw_call_manager)
		{}

		void Process(Chunk* chunk) override;
	};

}