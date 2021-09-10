#pragma once

#include "render/shader/Shader.h"

namespace render::graph
{
	class RenderGraph;
	class IRenderPassBuilder;
	struct DependencyNode;
	struct ResourceWrapper;
}

namespace Device
{
	class VulkanRenderTargetAttachment;
	class Texture;
	class ShaderCache;
	class ResourceBindings;
	class ConstantBindings;
}

namespace render
{
	struct EnvironmentSettings;
};

class Mesh;

namespace render
{
	class Blur;

	class Bloom
	{
	public:
		Bloom(Device::ShaderCache& shader_cache, Blur& blur, EnvironmentSettings& environment_settings);

		void PrepareRendering(render::graph::RenderGraph& graph);
		graph::DependencyNode* AddBloom(graph::RenderGraph& graph, graph::DependencyNode& src_target_node, graph::ResourceWrapper& destination_target);
		void OnRecreateSwapchain(int32_t width, int32_t height);

	private:
		Blur& blur;

		std::array<std::unique_ptr<Device::VulkanRenderTargetAttachment>, 2> attachments;
		std::array<render::graph::ResourceWrapper*, 2> attachment_wrappers;

		std::unique_ptr<Mesh> full_screen_quad_mesh;
		Device::ShaderProgram* shader_resample;
		Device::ShaderProgram* shader_blend;

		EnvironmentSettings& environment_settings;
	};

}