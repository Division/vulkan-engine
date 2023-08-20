#include "Blur.h"

#include "CommonIncludes.h"
#include "render/shader/Shader.h"
#include "render/renderer/SceneRenderer.h"
#include "render/device/Device.h"
#include "render/texture/Texture.h"
#include "render/mesh/Mesh.h"
#include "render/shader/ShaderCache.h"
#include "ecs/components/DrawCall.h"
#include "Engine.h"
#include <numeric>
#include <algorithm>
#include <filesystem>

RPS_DECLARE_RPSL_ENTRY(blur, blur);

namespace Modules
{
	using namespace Device;

	Blur::Blur(const char* shader_dir)
	{
		const auto shaderPath = std::filesystem::path(shader_dir).append("blur.hlsl");

		auto blur_shader_info = ShaderProgramInfo()
			.AddShader(ShaderProgram::Stage::Vertex, shaderPath.wstring(), "VSMain")
			.AddShader(ShaderProgram::Stage::Fragment, shaderPath.wstring(), "PSMainBlur");

		shader_blur = Engine::Get()->GetShaderCache()->GetShaderProgram(blur_shader_info);

		full_screen_quad_mesh = Engine::Get()->GetSceneRenderer()->GetRendererResources().full_screen_quad_mesh.get();

        RpsProgramCreateInfo programCreateInfo = {};
        programCreateInfo.hRpslEntryPoint      = RPS_ENTRY_REF(blur, blur);

        RpsResult result = rpsProgramCreate(Engine::GetVulkanContext()->GetRpsDevice(), &programCreateInfo, &program);
		assert(result == RPS_OK);
		result = rpsProgramBindNode(program, "RenderBlur", &Blur::Render, this);
		assert(result == RPS_OK);
	}

	void Blur::Render(const RpsCmdCallbackContext* pContext)
	{
		auto& state = *static_cast<VulkanRenderState*>(pContext->pUserRecordContext);

		RenderMode mode;
		mode.SetDepthWriteEnabled(false);
		mode.SetDepthTestEnabled(false);
		mode.SetPolygonMode(PolygonMode::Fill);
		mode.SetPrimitiveTopology(PrimitiveTopology::TriangleList);

		std::vector<float> gaussian_values(16, 0.0f);
		std::vector<vec4> sample_offsets(16, vec4(0.0f));

		// Arguments
		const uvec2 viewport_size = *rpsCmdGetArg<uvec2, 1>(pContext);
		const vec2 blur_direction_sigma = *rpsCmdGetArg<vec2, 2>(pContext);
		VkImageView srcImageView = {};
		if (rpsVKGetCmdArgImageView(pContext, 3, &srcImageView) != RPS_OK)
			throw std::runtime_error("failed getting arg");

		const float min_sigma = 0.5f;
		const float max_sigma = 4.0f;
		const float sigma = std::max(min_sigma, std::min(std::max(blur_direction_sigma.x, blur_direction_sigma.y), max_sigma));
		const int radius = (int)round(2 * sigma);

		for (int j = 0; j <= radius; j++)
		{
			assert(gaussian_values.size() > j);
			float value = 1.0f / (sqrt(2 * PI) * sigma) * exp(-j * j / (2 * sigma * sigma));
			gaussian_values[j] = value;
			sample_offsets[j] = vec4((float)j, (float)j, 0.0f, 0.0f) / vec4(viewport_size, 1, 1);
		}

		float summ = std::accumulate(gaussian_values.begin(), gaussian_values.end(), 0.0f);
		summ = std::accumulate(gaussian_values.begin() + 1, gaussian_values.end(), summ);
		for (auto& value : gaussian_values)
			value /= summ;

		VkRenderPass renderPassFromRps = {};
		rpsVKGetCmdRenderPass(pContext, &renderPassFromRps);
		auto renderPass = Device::VulkanRenderPass(Device::VulkanRenderPassInitializer(renderPassFromRps));
		static VulkanPipeline pipeline(VulkanPipelineInitializer(shader_blur, &renderPass, &full_screen_quad_mesh->GetVertexLayout(), &mode));
		state.BindPipeline(pipeline);

		vec2 blur_direction(blur_direction_sigma.x > 0.1f ? (float)radius : 0.0f, blur_direction_sigma.y > 0.1f ? (float)radius : 0.0f);

		ConstantBindings constants;
		constants.AddFloat2Binding(&blur_direction, "blur_direction");
		constants.AddDataBinding(sample_offsets.data(), sample_offsets.size() * sizeof(float4), "blur_offsets");
		constants.AddDataBinding(gaussian_values.data(), gaussian_values.size() * sizeof(float), "blur_weights");

		ResourceBindings resource_bindings;

		resource_bindings.AddImageViewBinding("src_texture", srcImageView);
		const auto* descriptor_set_layout = shader_blur->GetDescriptorSetLayout(0);
		const DescriptorSetBindings bindings(resource_bindings, *descriptor_set_layout);

		state.SetDescriptorSetBindings(bindings, constants);

		state.Draw(*full_screen_quad_mesh->vertexBuffer(), full_screen_quad_mesh->indexCount(), 0);
	}

}