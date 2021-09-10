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
	class Blur
	{
	public:
		Blur(Device::ShaderCache& shader_cache);

		graph::DependencyNode* AddOneDirectionBlur(graph::RenderGraph& graph, graph::DependencyNode& src_target_node, graph::ResourceWrapper& destination_target, vec2 blur_direction_sigma);
		graph::DependencyNode* AddFullBlur(graph::RenderGraph& graph, graph::DependencyNode& src_target_node, graph::ResourceWrapper& intermediate_target, graph::ResourceWrapper& destination_target, float sigma);

	private:
		std::unique_ptr<Mesh> full_screen_quad_mesh;
		Device::ShaderProgram* shader_blur;
	};

}