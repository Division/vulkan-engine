#include "CommonIncludes.h"
#include "PostProcess.h"
#include "render/renderer/RenderGraph.h"
#include "render/renderer/SceneRenderer.h"
#include "render/device/VulkanPipeline.h"
#include "render/device/VkObjects.h"
#include "render/device/VulkanRenderTarget.h"
#include "render/device/VulkanRenderState.h"
#include "render/device/VulkanDescriptorCache.h"
#include "render/shader/Shader.h"
#include "render/buffer/VulkanBuffer.h"
#include "render/texture/Texture.h"
#include "render/mesh/Mesh.h"
#include "render/renderer/EnvironmentSettings.h"
#include "utils/MeshGeneration.h"
#include "ecs/components/DrawCall.h"
#include "Engine.h"

namespace render::effects 

{
	using namespace Device;
	using namespace render::graph;
	using namespace ECS;
	using namespace profiler;

	Device::ShaderProgramInfo PostProcess::GetShaderInfo(const PostProcessSettings& settings)
	{
		std::vector<ShaderProgramInfo::Macro> defines;

		if (settings.bloom_enabled)
			defines.push_back({ "BLOOM_ENABLED", "" });

		return ShaderProgramInfo()
			.AddShader(ShaderProgram::Stage::Vertex, L"shaders/postprocess/postprocess.hlsl", "vs_main", defines)
			.AddShader(ShaderProgram::Stage::Fragment, L"shaders/postprocess/postprocess.hlsl", "ps_main", defines);
	}

	Device::ShaderProgram* PostProcess::GetShader(const PostProcessSettings& settings)
	{
		auto it = shaders.find(settings);
		if (it == shaders.end())
			it = shaders.insert({ settings, shader_cache.GetShaderProgram(GetShaderInfo(settings)) }).first;

		return it->second;
	}

	PostProcess::PostProcess(ShaderCache& shader_cache, EnvironmentSettings& environment_settings, RendererResources& render_resources)
		: environment_settings(environment_settings)
		, shader_cache(shader_cache)
	{
		const auto shader_info_base = ShaderProgramInfo()
			.AddShader(ShaderProgram::Stage::Vertex, L"shaders/postprocess/postprocess.hlsl", "vs_main")
			.AddShader(ShaderProgram::Stage::Fragment, L"shaders/postprocess/postprocess.hlsl", "ps_main");

		PostProcessSettings base;

		full_screen_quad_mesh = render_resources.full_screen_quad_mesh.get();
	}

	void PostProcess::PrepareRendering(RenderGraph& graph)
	{
		attachment_wrappers[0] = graph.RegisterAttachment(*attachments[0]);
		attachment_wrappers[1] = graph.RegisterAttachment(*attachments[1]);
	}

	graph::DependencyNode* PostProcess::AddPostProcess(RenderGraph& graph, const Input& input, ResourceWrapper& destination_target, const Device::ResourceBindings& global_bindings, const ConstantBindings& global_constants)
	{
		auto* destination_render_target = destination_target.GetAttachment();

		struct PassInfo
		{
			graph::DependencyNode* color_output = nullptr;
		};

		PostProcessSettings settings;
		settings.bloom_enabled = (bool)input.bloom_texture;
		auto* shader = GetShader(settings);

		auto post_process_info = graph.AddPass<PassInfo>("Post Process", ProfilerName::PassPostProcess, [&](graph::IRenderPassBuilder& builder)
			{
				PassInfo result;
				builder.AddInput(*input.src_texture);
				if (input.bloom_texture)
					builder.AddInput(*input.bloom_texture);

				result.color_output = builder.AddOutput(destination_target);
				return result;
			}, [&, shader](VulkanRenderState& state)
			{
				RenderMode mode;
				mode.SetDepthWriteEnabled(false);
				mode.SetDepthTestEnabled(false);
				mode.SetPolygonMode(PolygonMode::Fill);
				mode.SetPrimitiveTopology(PrimitiveTopology::TriangleList);

				ResourceBindings resource_bindings;
				resource_bindings.AddTextureBinding("src_texture", input.src_texture->resource->GetAttachment()->GetTexture().get());
				if (input.bloom_texture)
					resource_bindings.AddTextureBinding("bloom_texture", input.bloom_texture->resource->GetAttachment()->GetTexture().get());


				const auto* descriptor_set_layout = shader->GetDescriptorSetLayout(0);
				const DescriptorSetBindings bindings(resource_bindings, *descriptor_set_layout);
				
				auto command_buffer = state.GetCurrentCommandBuffer()->GetCommandBuffer();

				ConstantBindings constants = global_constants;
				constants.AddFloatBinding(&environment_settings.exposure, "exposure");
				constants.AddFloatBinding(&environment_settings.bloom_strength, "bloom_strength"); 
				const vec4 bloom_tex_size = input.bloom_texture ? input.bloom_texture->resource->GetAttachment()->GetTexSize() : vec4(0);
				constants.AddFloat4Binding(&bloom_tex_size, "bloom_tex_size");
				state.SetGlobalBindings(global_bindings, constants);

				state.SetRenderMode(mode);
				state.SetShader(*shader);
				state.SetVertexLayout(full_screen_quad_mesh->GetVertexLayout());
				state.UpdateState();

				state.SetDescriptorSetBindings(bindings, constants);
				state.Draw(*full_screen_quad_mesh->vertexBuffer(), full_screen_quad_mesh->indexCount(), 0);
			});

		current_target = (current_target + 1) % attachments.size();

		return post_process_info.color_output;
	}

	void PostProcess::OnRecreateSwapchain(int32_t width, int32_t height)
	{
		attachments[0] = std::make_unique<VulkanRenderTargetAttachment>("PostProcess 0", VulkanRenderTargetAttachment::Type::Color, width, height, Format::R16G16B16A16_float);
		attachments[1] = std::make_unique<VulkanRenderTargetAttachment>("PostProcess 1", VulkanRenderTargetAttachment::Type::Color, width, height, Format::R16G16B16A16_float);
	}

}