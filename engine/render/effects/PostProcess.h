#pragma once

#include "render/shader/Shader.h"

namespace core::render::graph
{
	class RenderGraph;
	class IRenderPassBuilder;
	struct DependencyNode;
	struct ResourceWrapper;
}

namespace core::Device
{
	class VulkanRenderTargetAttachment;
	class Texture;
	class ShaderCache;
}

namespace core::render
{
	struct EnvironmentSettings;
};

class Mesh;

namespace core { namespace render { namespace effects {

	class PostProcess
	{
	public:
		PostProcess(core::Device::ShaderCache& shader_cache, EnvironmentSettings& environment_settings);

		void PrepareRendering(core::render::graph::RenderGraph& graph);
		core::render::graph::DependencyNode* AddPostProcess(
			core::render::graph::RenderGraph& graph, 
			core::render::graph::DependencyNode& src_target_node, 
			core::render::graph::ResourceWrapper& destination_target, 
			core::render::graph::ResourceWrapper& hdr_buffer
		);
		void OnRecreateSwapchain(int32_t width, int32_t height);

	private:
		std::array<std::unique_ptr<core::Device::VulkanRenderTargetAttachment>, 2> attachments;
		std::array<core::render::graph::ResourceWrapper*, 2> attachment_wrappers;
		uint32_t current_target = 0;

		core::Device::ShaderProgram::BindingAddress src_texture_address;
		core::Device::ShaderProgram::BindingAddress hdr_buffer_address;

		core::Device::Texture* cubemap_texture = nullptr;
		std::unique_ptr<Mesh> full_screen_quad_mesh;
		core::Device::ShaderProgram* shader;

		EnvironmentSettings& environment_settings;
	};

} } }