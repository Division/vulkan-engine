#pragma once

#include "render/shader/Shader.h"
#include "render/shader/ShaderCache.h"

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

namespace render { namespace effects {

	class PostProcess
	{
	public:
		struct Input
		{
			graph::DependencyNode* src_texture;
			graph::DependencyNode* bloom_texture;
		};

#pragma pack(push, 1)
		struct PostProcessSettings
		{
			bool bloom_enabled = false;

			bool operator==(const PostProcessSettings& other) const { return std::tie(bloom_enabled) == std::tie(other.bloom_enabled); }

			struct Hasher
			{
				size_t operator()(const PostProcessSettings& v) const { return FastHash(&v, sizeof(v)); }
			};
		};
#pragma pack(pop)

		PostProcess(Device::ShaderCache& shader_cache, EnvironmentSettings& environment_settings);

		void PrepareRendering(render::graph::RenderGraph& graph);
		render::graph::DependencyNode* AddPostProcess(
			render::graph::RenderGraph& graph, 
			const Input& input,
			render::graph::ResourceWrapper& destination_target, 
			render::graph::ResourceWrapper& hdr_buffer,
			const Device::ResourceBindings& global_bindings,
			const Device::ConstantBindings& global_constants
		);
		void OnRecreateSwapchain(int32_t width, int32_t height);

	private:
		Device::ShaderProgramInfo GetShaderInfo(const PostProcessSettings& settings);
		Device::ShaderProgram* GetShader(const PostProcessSettings& settings);

		Device::ShaderCache& shader_cache;
		std::array<std::unique_ptr<Device::VulkanRenderTargetAttachment>, 2> attachments;
		std::array<render::graph::ResourceWrapper*, 2> attachment_wrappers;
		uint32_t current_target = 0;

		Device::ShaderProgram::BindingAddress src_texture_address;
		Device::ShaderProgram::BindingAddress hdr_buffer_address;

		Device::Texture* cubemap_texture = nullptr;
		std::unique_ptr<Mesh> full_screen_quad_mesh;
		std::unordered_map<PostProcessSettings, Device::ShaderProgram*, PostProcessSettings::Hasher> shaders;

		EnvironmentSettings& environment_settings;
	};

} }