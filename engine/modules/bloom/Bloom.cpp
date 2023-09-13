#include "Bloom.h"

#include <filesystem>
#include "CommonIncludes.h"
#include "render/device/VulkanPipeline.h"
#include "render/device/VulkanRenderPass.h"
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

RPS_DECLARE_RPSL_ENTRY(bloom, main);

namespace Modules
{
	using namespace Device;
	using namespace ECS;

	Bloom::Bloom(const char* shader_dir, BloomSettings* bloom_settings)
		: bloom_settings(*bloom_settings)
	{
		const auto shaderPath = std::filesystem::path(shader_dir).append("bloom.hlsl");
		auto& shader_cache = *Engine::Get()->GetShaderCache();

		auto prefilter_shader_info = ShaderProgramInfo()
			.AddShader(ShaderProgram::Stage::Vertex, shaderPath.wstring(), "vs_main")
			.AddShader(ShaderProgram::Stage::Fragment, shaderPath.wstring(), "ps_prefilter_main");

		shader_prefilter = shader_cache.GetShaderProgram(prefilter_shader_info);

		auto blur_h_info = ShaderProgramInfo()
			.AddShader(ShaderProgram::Stage::Vertex, shaderPath.wstring(), "vs_main")
			.AddShader(ShaderProgram::Stage::Fragment, shaderPath.wstring(), "ps_blur_h_main");

		shader_blur_h = shader_cache.GetShaderProgram(blur_h_info);

		auto blur_v_info = ShaderProgramInfo()
			.AddShader(ShaderProgram::Stage::Vertex, shaderPath.wstring(), "vs_main")
			.AddShader(ShaderProgram::Stage::Fragment, shaderPath.wstring(), "ps_blur_v_main");

		shader_blur_v = shader_cache.GetShaderProgram(blur_v_info);

		auto upsample_info = ShaderProgramInfo()
			.AddShader(ShaderProgram::Stage::Vertex, shaderPath.wstring(), "vs_main")
			.AddShader(ShaderProgram::Stage::Fragment, shaderPath.wstring(), "ps_upsample_main");

		shader_upsample = shader_cache.GetShaderProgram(upsample_info);

		auto& render_resources = Engine::Get()->GetSceneRenderer()->GetRendererResources();
		full_screen_quad_mesh = render_resources.full_screen_quad_mesh.get();

		RpsProgramCreateInfo programCreateInfo = {};
		programCreateInfo.hRpslEntryPoint = RPS_ENTRY_REF(bloom, main);
		program = render::RpsSubprogramHandle(programCreateInfo);

		auto result = rpsProgramBindNode(*program, "RenderPrefilter", &Bloom::RenderPrefilter, this);
		assert(result == RPS_OK);
		//result = rpsProgramBindNode(*program, "RenderBlurH", &Bloom::RenderBlurH, this);
		//assert(result == RPS_OK);
		//result = rpsProgramBindNode(*program, "RenderBlurV", &Bloom::RenderBlurV, this);
		//assert(result == RPS_OK);
		//result = rpsProgramBindNode(*program, "RenderUpsample", &Bloom::RenderUpsample, this);
		//assert(result == RPS_OK);
	}

	void Bloom::RenderCommon(const RpsCmdCallbackContext* pContext, VulkanPipeline& pipeline, vk::ImageView srcTextureLow, vec4 lowTexSize)
	{
		auto& state = *static_cast<VulkanRenderState*>(pContext->pUserRecordContext);

		VkImageView srcImageView = {};
		if (rpsVKGetCmdArgImageView(pContext, 1, &srcImageView) != RPS_OK)
			throw std::runtime_error("failed getting arg");

		const uvec2 viewport_size = *rpsCmdGetArg<uvec2, 2>(pContext);
		const float bloomThreshold = *rpsCmdGetArg<float, 3>(pContext);
		const float bloomStrength = *rpsCmdGetArg<float, 4>(pContext);
		const float scatter = *rpsCmdGetArg<float, 5>(pContext);
		const float clampMax = *rpsCmdGetArg<float, 6>(pContext);
		const float thresholdKnee = bloomThreshold / 2.0f;

		ResourceBindings resource_bindings;
		resource_bindings.AddImageViewBinding("src_texture", srcImageView);
		if (srcTextureLow)
			resource_bindings.AddImageViewBinding("src_texture_low", srcTextureLow);

		ConstantBindings constants;
		constants.AddFloatBinding(&bloomThreshold, "bloom_threshold");
		constants.AddFloatBinding(&bloomStrength, "bloom_strength");
		constants.AddFloatBinding(&scatter, "Scatter");
		constants.AddFloatBinding(&clampMax, "ClampMax");
		constants.AddFloat4Binding(&lowTexSize, "low_tex_size");
		constants.AddFloatBinding(&thresholdKnee, "ThresholdKnee");

		const vec2 texel_size = vec2(1.0f) / vec2(viewport_size.x, viewport_size.y);
		constants.AddFloat2Binding(&texel_size, "texel_size");

		state.BindPipeline(pipeline);
		const auto* descriptor_set_layout = pipeline.GetShaderProgram()->GetDescriptorSetLayout(0);
		const DescriptorSetBindings bindings(resource_bindings, *descriptor_set_layout);
		state.SetDescriptorSetBindings(bindings, constants);

		state.Draw(*full_screen_quad_mesh->vertexBuffer(), full_screen_quad_mesh->indexCount(), 0);
	}


	void Bloom::RenderPrefilter(const RpsCmdCallbackContext* pContext)
	{
		RenderMode mode;
		mode.SetDepthWriteEnabled(false);
		mode.SetDepthTestEnabled(false);
		mode.SetPolygonMode(PolygonMode::Fill);
		mode.SetPrimitiveTopology(PrimitiveTopology::TriangleList);

		VkRenderPass renderPassFromRps = {};
		rpsVKGetCmdRenderPass(pContext, &renderPassFromRps);
		auto renderPass = VulkanRenderPass(VulkanRenderPassInitializer(renderPassFromRps));
		static VulkanPipeline pipeline(VulkanPipelineInitializer(shader_prefilter, &renderPass, &full_screen_quad_mesh->GetVertexLayout(), &mode));

		RenderCommon(pContext, pipeline);
	}


	void Bloom::RenderBlurH(const RpsCmdCallbackContext* pContext)
	{
		//RenderCommon(pContext, nullptr);
	}


	void Bloom::RenderBlurV(const RpsCmdCallbackContext* pContext)
	{
		//RenderCommon(pContext, nullptr);
	}


	void Bloom::RenderUpsample(const RpsCmdCallbackContext* pContext)
	{
		//RenderCommon(pContext, nullptr);
	}


	//DependencyNode* Bloom::AddBloom(RenderGraph& graph, DependencyNode& src_target_node, ResourceWrapper& destination_target)
	//{
	//	struct PassInfo
	//	{
	//		DependencyNode* color_output = nullptr;
	//	};
	//	auto prefilter_output = AddPass("Bloom Prefilter", *shader_prefilter, graph, src_target_node, nullptr, *mips[0].downsamle_wrapper);
	//	DependencyNode* blur_output = prefilter_output;
	//	mips[0].downsample_node = prefilter_output;
	//	for (uint32_t i = 1; i < mips.size(); i++)
	//	{
	//		auto blur_H_output = AddPass(("Bloom blur H " + std::to_string(i)).c_str(), *shader_blur_h, graph, *blur_output, nullptr, *mips[i].upsamle_wrapper);
	//		blur_output = AddPass(("Bloom blur V " + std::to_string(i)).c_str(), *shader_blur_v, graph, *blur_H_output, nullptr, *mips[i].downsamle_wrapper);
	//		mips[i].downsample_node = blur_output;
	//	}
	//	const uint32_t last_mip_index = mips.size() - 1;
	//	mips[last_mip_index - 1].upsample_node = AddPass(("Bloom Upsample " + std::to_string(last_mip_index - 1)).c_str(), *shader_upsample, graph, *mips[last_mip_index - 1].downsample_node, mips[last_mip_index].downsample_node, *mips[last_mip_index - 1].upsamle_wrapper);
	//	for (int i = last_mip_index - 2; i >= 0; i--)
	//	{
	//		mips[i].upsample_node = AddPass(("Bloom Upsample " + std::to_string(i)).c_str(), *shader_upsample, graph, *mips[i].downsample_node, mips[i + 1].upsample_node, *mips[i].upsamle_wrapper);
	//	}
	//	return mips[0].upsample_node;
	//}
	//void Bloom::OnRecreateSwapchain(int32_t width, int32_t height)
	//{
	//	mips.clear();
	//	int32_t mip_width = width / 2;
	//	int32_t mip_height = height / 2;
	//	uint32_t index = 0;
	//	while (true)
	//	{
	//		if (mip_width < 2 || mip_height < 2)
	//			break;
	//		auto& mip = mips.emplace_back();
	//		mip.downsamle = std::make_unique<VulkanRenderTargetAttachment>("Bloom Downsample " + std::to_string(index), VulkanRenderTargetAttachment::Type::Color, mip_width, mip_height, Format::R16G16B16A16_float);
	//		mip.upsample = std::make_unique<VulkanRenderTargetAttachment>("Bloom Upsample " + std::to_string(index), VulkanRenderTargetAttachment::Type::Color, mip_width, mip_height, Format::R16G16B16A16_float);
	//		index++;
	//		mip_width /= 2;
	//		mip_height /= 2;
	//	}
	//}

}