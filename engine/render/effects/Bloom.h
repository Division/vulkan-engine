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

class Mesh;

namespace render
{
	class Blur;
	struct EnvironmentSettings;
	struct RendererResources;

	class Bloom
	{
	public:
		Bloom(Device::ShaderCache& shader_cache, Blur& blur, EnvironmentSettings& environment_settings, RendererResources& render_resources);

		void PrepareRendering(render::graph::RenderGraph& graph);
		graph::DependencyNode* AddBloom(graph::RenderGraph& graph, graph::DependencyNode& src_target_node, graph::ResourceWrapper& destination_target);
		void OnRecreateSwapchain(int32_t width, int32_t height);

	private:
		graph::DependencyNode* AddPass(const char* name, Device::ShaderProgram& shader, graph::RenderGraph& graph, graph::DependencyNode& src_target_node, graph::DependencyNode* src_low_res_node, graph::ResourceWrapper& destination_target);

	private:

		Blur& blur;

		struct Mip
		{
			std::unique_ptr<Device::VulkanRenderTargetAttachment> downsamle;
			std::unique_ptr<Device::VulkanRenderTargetAttachment> upsample;
			render::graph::ResourceWrapper* downsamle_wrapper;
			render::graph::ResourceWrapper* upsamle_wrapper;
			graph::DependencyNode* downsample_node;
			graph::DependencyNode* upsample_node;
		};

		std::vector<Mip> mips;
		Mesh* full_screen_quad_mesh;
		Device::ShaderProgram* shader_prefilter;
		Device::ShaderProgram* shader_blur_h;
		Device::ShaderProgram* shader_blur_v;
		Device::ShaderProgram* shader_upsample;

		EnvironmentSettings& environment_settings;
	};

}