#include "Bloom.h"

#include "CommonIncludes.h"
#include "render/renderer/RenderGraph.h"
#include "render/device/VulkanPipeline.h"
#include "render/device/VkObjects.h"
#include "render/device/VulkanRenderTarget.h"
#include "render/device/VulkanRenderState.h"
#include "render/device/VulkanDescriptorCache.h"
#include "render/shader/Shader.h"
#include "render/buffer/VulkanBuffer.h"
#include "render/texture/Texture.h"
#include "render/mesh/Mesh.h"
#include "render/shader/ShaderCache.h"
#include "render/renderer/EnvironmentSettings.h"
#include "utils/MeshGeneration.h"
#include "ecs/components/DrawCall.h"
#include "Engine.h"
#include "Blur.h"

namespace render
{
	using namespace Device;
	using namespace graph;
	using namespace ECS;
	using namespace profiler;

	Bloom::Bloom(ShaderCache& shader_cache, Blur& blur, EnvironmentSettings& environment_settings)
		: environment_settings(environment_settings)
		, blur(blur)
	{
		auto resample_shader_info = ShaderProgramInfo()
			.AddShader(ShaderProgram::Stage::Vertex, L"shaders/bloom.hlsl", "vs_main")
			.AddShader(ShaderProgram::Stage::Fragment, L"shaders/bloom.hlsl", "ps_resample_main");

		shader_resample = shader_cache.GetShaderProgram(resample_shader_info);

		auto blend_shader_info = ShaderProgramInfo()
			.AddShader(ShaderProgram::Stage::Vertex, L"shaders/bloom.hlsl", "vs_main")
			.AddShader(ShaderProgram::Stage::Fragment, L"shaders/bloom.hlsl", "ps_blend_main");

		shader_blend = shader_cache.GetShaderProgram(blend_shader_info);

		full_screen_quad_mesh = std::make_unique<Mesh>(false);
		MeshGeneration::generateFullScreenQuad(full_screen_quad_mesh.get());
		full_screen_quad_mesh->createBuffer();
	}

	void Bloom::PrepareRendering(RenderGraph& graph)
	{
		attachment_wrappers[0] = graph.RegisterAttachment(*attachments[0]);
		attachment_wrappers[1] = graph.RegisterAttachment(*attachments[1]);
	}

	DependencyNode* Bloom::AddBloom(RenderGraph& graph, DependencyNode& src_target_node, ResourceWrapper& destination_target)
	{
		struct PassInfo
		{
			DependencyNode* color_output = nullptr;
		};

		auto downsample_info = graph.AddPass<PassInfo>("Bloom downsample", ProfilerName::PassPostProcess, [&](graph::IRenderPassBuilder& builder)
			{
				PassInfo result;
				builder.AddInput(src_target_node);
				result.color_output = builder.AddOutput(*attachment_wrappers[0]);
				return result;
			}, [&](VulkanRenderState& state)
			{
				RenderMode mode;
				mode.SetDepthWriteEnabled(false);
				mode.SetDepthTestEnabled(false);
				mode.SetPolygonMode(PolygonMode::Fill);
				mode.SetPrimitiveTopology(PrimitiveTopology::TriangleList);

				ResourceBindings resource_bindings;
				resource_bindings.AddTextureBinding("src_texture", src_target_node.resource->GetAttachment()->GetTexture().get());
				const auto* descriptor_set_layout = shader_resample->GetDescriptorSetLayout(0);
				const DescriptorSetBindings bindings(resource_bindings, *descriptor_set_layout);

				auto command_buffer = state.GetCurrentCommandBuffer()->GetCommandBuffer();

				ConstantBindings constants;
				constants.AddFloatBinding(&environment_settings.bloom_threshold, "bloom_threshold");
				constants.AddFloatBinding(&environment_settings.bloom_strength, "bloom_strength");
				state.SetGlobalBindings(resource_bindings, constants);

				state.SetRenderMode(mode);
				state.SetShader(*shader_resample);
				state.SetVertexLayout(full_screen_quad_mesh->GetVertexLayout());
				state.UpdateState();
				state.SetDescriptorSetBindings(bindings, constants);

				state.Draw(*full_screen_quad_mesh->vertexBuffer(), full_screen_quad_mesh->indexCount(), 0);
			});

		auto* blur_output = blur.AddFullBlur(graph, *downsample_info.color_output, *attachment_wrappers[1], *attachment_wrappers[0], environment_settings.bloom_sigma);

		auto bloom_info = graph.AddPass<PassInfo>("Bloom blend", ProfilerName::PassPostProcess, [&, blur_output](graph::IRenderPassBuilder& builder)
			{
				PassInfo result;
				builder.AddInput(*blur_output);
				result.color_output = builder.AddOutput(destination_target);
				return result;
			}, [&, blur_output](VulkanRenderState& state)
			{
				RenderMode mode;
				mode.SetDepthWriteEnabled(false);
				mode.SetDepthTestEnabled(false);
				mode.SetPolygonMode(PolygonMode::Fill);
				mode.SetPrimitiveTopology(PrimitiveTopology::TriangleList);
				
				ResourceBindings resource_bindings;
				resource_bindings.AddTextureBinding("src_texture", src_target_node.resource->GetAttachment()->GetTexture().get());
				resource_bindings.AddTextureBinding("src_bloom", blur_output->resource->GetAttachment()->GetTexture().get());
				const auto* descriptor_set_layout = shader_blend->GetDescriptorSetLayout(0);
				const DescriptorSetBindings bindings(resource_bindings, *descriptor_set_layout);

				auto command_buffer = state.GetCurrentCommandBuffer()->GetCommandBuffer();

				ConstantBindings constants;
				constants.AddFloatBinding(&environment_settings.exposure, "exposure");
				state.SetGlobalBindings(resource_bindings, constants);

				state.SetRenderMode(mode);
				state.SetShader(*shader_blend);
				state.SetVertexLayout(full_screen_quad_mesh->GetVertexLayout());
				state.UpdateState();
				state.SetDescriptorSetBindings(bindings, constants);

				state.Draw(*full_screen_quad_mesh->vertexBuffer(), full_screen_quad_mesh->indexCount(), 0);
			});

		return bloom_info.color_output;
	}

	void Bloom::OnRecreateSwapchain(int32_t width, int32_t height)
	{
		attachments[0] = std::make_unique<VulkanRenderTargetAttachment>("Bloom 0", VulkanRenderTargetAttachment::Type::Color, width / 2, height / 2, Format::R16G16B16A16_float);
		attachments[1] = std::make_unique<VulkanRenderTargetAttachment>("Bloom 1", VulkanRenderTargetAttachment::Type::Color, width / 2, height / 2, Format::R16G16B16A16_float);
	}

}