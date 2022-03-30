#pragma once

#include <memory>

namespace Device
{
	class Texture;
	class VulkanRenderState;
	class ShaderProgram;
	class ShaderCache;
}

namespace ECS
{
	class EntityManager;
}

namespace ECS::systems
{
	class ParticleEmitSystem;
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
}

namespace render::effects {

	class GPUParticles
	{
	public:
		GPUParticles(SceneRenderer& scene_renderer, Device::ShaderCache& shader_cache, ECS::EntityManager& manager);
		~GPUParticles();

		void Update(graph::RenderGraph& graph, float dt);

	private:
		SceneRenderer& scene_renderer;
		ECS::EntityManager& manager;
		Device::ShaderProgram* emit_shader;
		Device::ShaderProgram* update_shader;
	};

}