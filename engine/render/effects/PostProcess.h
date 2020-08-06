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
}

namespace render
{
	struct EnvironmentSettings;
};

class Mesh;

namespace render { namespace effects {

	class PostProcess
	{
	public:
		PostProcess(Device::ShaderCache& shader_cache, EnvironmentSettings& environment_settings);

		void PrepareRendering(render::graph::RenderGraph& graph);
		render::graph::DependencyNode* AddPostProcess(
			render::graph::RenderGraph& graph, 
			render::graph::DependencyNode& src_target_node, 
			render::graph::ResourceWrapper& destination_target, 
			render::graph::ResourceWrapper& hdr_buffer
		);
		void OnRecreateSwapchain(int32_t width, int32_t height);

	private:
		std::array<std::unique_ptr<Device::VulkanRenderTargetAttachment>, 2> attachments;
		std::array<render::graph::ResourceWrapper*, 2> attachment_wrappers;
		uint32_t current_target = 0;

		Device::ShaderProgram::BindingAddress src_texture_address;
		Device::ShaderProgram::BindingAddress hdr_buffer_address;

		Device::Texture* cubemap_texture = nullptr;
		std::unique_ptr<Mesh> full_screen_quad_mesh;
		Device::ShaderProgram* shader;

		EnvironmentSettings& environment_settings;
	};

} }