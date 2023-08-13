#include "Bloom.h"

#include "CommonIncludes.h"
#include "render/renderer/RenderGraph.h"
#include "render/device/VulkanPipeline.h"
#include "render/device/VulkanRenderTarget.h"
#include "render/device/VulkanRenderState.h"
#include "render/device/VulkanDescriptorCache.h"
#include "render/shader/Shader.h"
#include "render/buffer/VulkanBuffer.h"
#include "render/texture/Texture.h"
#include "render/mesh/Mesh.h"
#include "render/shader/ShaderCache.h"
#include "render/renderer/EnvironmentSettings.h"
#include "render/renderer/SceneRenderer.h"
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

	Bloom::Bloom(ShaderCache& shader_cache, Blur& blur, EnvironmentSettings& environment_settings, RendererResources& render_resources)
		: environment_settings(environment_settings)
		, blur(blur)
	{
		auto prefilter_shader_info = ShaderProgramInfo()
			.AddShader(ShaderProgram::Stage::Vertex, L"shaders/bloom.hlsl", "vs_main")
			.AddShader(ShaderProgram::Stage::Fragment, L"shaders/bloom.hlsl", "ps_prefilter_main");

		shader_prefilter = shader_cache.GetShaderProgram(prefilter_shader_info);

		auto blur_h_info = ShaderProgramInfo()
			.AddShader(ShaderProgram::Stage::Vertex, L"shaders/bloom.hlsl", "vs_main")
			.AddShader(ShaderProgram::Stage::Fragment, L"shaders/bloom.hlsl", "ps_blur_h_main");

		shader_blur_h = shader_cache.GetShaderProgram(blur_h_info);

		auto blur_v_info = ShaderProgramInfo()
			.AddShader(ShaderProgram::Stage::Vertex, L"shaders/bloom.hlsl", "vs_main")
			.AddShader(ShaderProgram::Stage::Fragment, L"shaders/bloom.hlsl", "ps_blur_v_main");

		shader_blur_v = shader_cache.GetShaderProgram(blur_v_info);

		auto upsample_info = ShaderProgramInfo()
			.AddShader(ShaderProgram::Stage::Vertex, L"shaders/bloom.hlsl", "vs_main")
			.AddShader(ShaderProgram::Stage::Fragment, L"shaders/bloom.hlsl", "ps_upsample_main");

		shader_upsample = shader_cache.GetShaderProgram(upsample_info);

		full_screen_quad_mesh = render_resources.full_screen_quad_mesh.get();
	}

	void Bloom::PrepareRendering(RenderGraph& graph)
	{
		for (auto& mip : mips)
		{
			mip.downsamle_wrapper = graph.RegisterAttachment(*mip.downsamle);
			mip.upsamle_wrapper = graph.RegisterAttachment(*mip.upsample);
			mip.downsample_node = nullptr;
			mip.upsample_node = nullptr;
		}
	}

	DependencyNode* Bloom::AddPass(const char* name, Device::ShaderProgram& shader, RenderGraph& graph, DependencyNode& src_target_node, DependencyNode* src_low_res_node, ResourceWrapper& destination_target)
	{
		struct PassInfo
		{
			DependencyNode* color_output = nullptr;
		};

		auto pass_info = graph.AddPass<PassInfo>(name, ProfilerName::PassPostProcess, [&, src_low_res_node](graph::IRenderPassBuilder& builder)
		{
			PassInfo result;
			builder.AddInput(src_target_node);
			if (src_low_res_node)
				builder.AddInput(*src_low_res_node);

			result.color_output = builder.AddOutput(destination_target)->Clear(vec4(0));
			return result;
		}, [&, src_low_res_node](VulkanRenderState& state)
		{
			//RenderMode mode;
			//mode.SetDepthWriteEnabled(false);
			//mode.SetDepthTestEnabled(false);
			//mode.SetPolygonMode(PolygonMode::Fill);
			//mode.SetPrimitiveTopology(PrimitiveTopology::TriangleList);

			//ResourceBindings resource_bindings;
			//resource_bindings.AddTextureBinding("src_texture", src_target_node.resource->GetAttachment()->GetTexture().get());
			//if (src_low_res_node)
			//	resource_bindings.AddTextureBinding("src_texture_low", src_low_res_node->resource->GetAttachment()->GetTexture().get());

			//const auto* descriptor_set_layout = shader.GetDescriptorSetLayout(0);
			//const DescriptorSetBindings bindings(resource_bindings, *descriptor_set_layout);

			//auto command_buffer = state.GetCurrentCommandBuffer()->GetCommandBuffer();

			//ConstantBindings constants;
			//constants.AddFloatBinding(&environment_settings.bloom_threshold, "bloom_threshold");
			//constants.AddFloatBinding(&environment_settings.bloom_strength, "bloom_strength");
			//constants.AddFloatBinding(&environment_settings.bloom_scatter, "Scatter");
			//constants.AddFloatBinding(&environment_settings.bloom_clamp_max, "ClampMax");
			//const float4 low_tex_size = src_low_res_node ? src_low_res_node->resource->GetAttachment()->GetTexSize() : float4(0);
			//constants.AddFloat4Binding(&low_tex_size, "low_tex_size");
			//float ThresholdKnee = environment_settings.bloom_threshold / 2.0f;
			//constants.AddFloatBinding(&ThresholdKnee, "ThresholdKnee");

			//const vec2 texel_size = vec2(1.0f) / vec2(src_target_node.resource->GetAttachment()->GetWidth(), src_target_node.resource->GetAttachment()->GetHeight());
			//constants.AddFloat2Binding(&texel_size, "texel_size");
			//state.SetGlobalBindings(resource_bindings, constants);

			//state.SetRenderMode(mode);
			//state.SetShader(shader);
			//state.SetVertexLayout(full_screen_quad_mesh->GetVertexLayout());
			//state.UpdateState();
			//state.SetDescriptorSetBindings(bindings, constants);

			//state.Draw(*full_screen_quad_mesh->vertexBuffer(), full_screen_quad_mesh->indexCount(), 0);
		});
		
		return pass_info.color_output;
	}

	DependencyNode* Bloom::AddBloom(RenderGraph& graph, DependencyNode& src_target_node, ResourceWrapper& destination_target)
	{
		struct PassInfo
		{
			DependencyNode* color_output = nullptr;
		};

		auto prefilter_output = AddPass("Bloom Prefilter", *shader_prefilter, graph, src_target_node, nullptr, *mips[0].downsamle_wrapper);
		DependencyNode* blur_output = prefilter_output;
		mips[0].downsample_node = prefilter_output;
		for (uint32_t i = 1; i < mips.size(); i++)
		{
			auto blur_H_output = AddPass(("Bloom blur H " + std::to_string(i)).c_str(), *shader_blur_h, graph, *blur_output, nullptr, *mips[i].upsamle_wrapper);
			blur_output = AddPass(("Bloom blur V " + std::to_string(i)).c_str(), *shader_blur_v, graph, *blur_H_output, nullptr, *mips[i].downsamle_wrapper);
			mips[i].downsample_node = blur_output;
		}

		const uint32_t last_mip_index = mips.size() - 1;

		mips[last_mip_index - 1].upsample_node = AddPass(("Bloom Upsample " + std::to_string(last_mip_index - 1)).c_str(), *shader_upsample, graph, *mips[last_mip_index - 1].downsample_node, mips[last_mip_index].downsample_node, *mips[last_mip_index - 1].upsamle_wrapper);
		for (int i = last_mip_index - 2; i >= 0; i--)
		{
			mips[i].upsample_node = AddPass(("Bloom Upsample " + std::to_string(i)).c_str(), *shader_upsample, graph, *mips[i].downsample_node, mips[i + 1].upsample_node, *mips[i].upsamle_wrapper);
		}


		return mips[0].upsample_node;
	}

	void Bloom::OnRecreateSwapchain(int32_t width, int32_t height)
	{
		mips.clear();

		int32_t mip_width = width / 2;
		int32_t mip_height = height / 2;
		uint32_t index = 0;
		while (true)
		{
			if (mip_width < 2 || mip_height < 2)
				break;

			auto& mip = mips.emplace_back();
			mip.downsamle = std::make_unique<VulkanRenderTargetAttachment>("Bloom Downsample " + std::to_string(index), VulkanRenderTargetAttachment::Type::Color, mip_width, mip_height, Format::R16G16B16A16_float);
			mip.upsample = std::make_unique<VulkanRenderTargetAttachment>("Bloom Upsample " + std::to_string(index), VulkanRenderTargetAttachment::Type::Color, mip_width, mip_height, Format::R16G16B16A16_float);

			index++;
			mip_width /= 2;
			mip_height /= 2;
		}
	}

}