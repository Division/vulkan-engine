#include "Blur.h"

#include "CommonIncludes.h"
#include "render/renderer/RenderGraph.h"
#include "render/renderer/SceneRenderer.h"
#include "render/device/VulkanPipeline.h"
#include "render/device/VulkanRenderTarget.h"
#include "render/device/VulkanRenderState.h"
#include "render/device/VulkanDescriptorCache.h"
#include "render/shader/Shader.h"
#include "render/buffer/VulkanBuffer.h"
#include "render/texture/Texture.h"
#include "render/mesh/Mesh.h"
#include "render/shader/ShaderCache.h"
#include "utils/MeshGeneration.h"
#include "ecs/components/DrawCall.h"
#include "Engine.h"
#include <numeric>
#include <algorithm>

namespace render
{
	using namespace Device;
	using namespace graph;
	using namespace ECS;
	using namespace profiler;

	Blur::Blur(ShaderCache& shader_cache, RendererResources& render_resources)
	{
		auto blur_shader_info = ShaderProgramInfo()
			.AddShader(ShaderProgram::Stage::Vertex, L"shaders/blur.hlsl", "VSMain")
			.AddShader(ShaderProgram::Stage::Fragment, L"shaders/blur.hlsl", "PSMainBlur");

		shader_blur = shader_cache.GetShaderProgram(blur_shader_info);

		full_screen_quad_mesh = render_resources.full_screen_quad_mesh.get();
	}

	DependencyNode* Blur::AddOneDirectionBlur(RenderGraph& graph, DependencyNode& src_target_node, ResourceWrapper& destination_target, vec2 blur_direction_sigma)
	{
		struct PassInfo
		{
			DependencyNode* color_output = nullptr;
		};

		const char* pass_name = blur_direction_sigma.x > blur_direction_sigma.y ? "Blur Horizontal" : "Blur Vertical";
		auto blur_info = graph.AddPass<PassInfo>(pass_name, ProfilerName::PassPostProcess, [&](graph::IRenderPassBuilder& builder)
			{
				PassInfo result;
				builder.AddInput(src_target_node);
				result.color_output = builder.AddOutput(destination_target)->Clear(0);
				return result;
			}, [&, blur_direction_sigma](VulkanRenderState& state)
			{
				//RenderMode mode;
				//mode.SetDepthWriteEnabled(false);
				//mode.SetDepthTestEnabled(false);
				//mode.SetPolygonMode(PolygonMode::Fill);
				//mode.SetPrimitiveTopology(PrimitiveTopology::TriangleList);

				//std::vector<float> gaussian_values(16, 0.0f);
				//std::vector<vec4> sample_offsets(16, vec4(0.0f));

				//const float min_sigma = 0.5f;
				//const float max_sigma = 4.0f;
				//const float sigma = std::max(min_sigma, std::min(std::max(blur_direction_sigma.x, blur_direction_sigma.y), max_sigma));
				//const int radius = (int)round(2 * sigma);
				//const vec2 viewport_size = vec2(destination_target.GetAttachment()->GetWidth(), destination_target.GetAttachment()->GetHeight());

				//for (int j = 0; j <= radius; j++)
				//{
				//	assert(gaussian_values.size() > j);
				//	float value = 1.0f / (sqrt(2 * PI) * sigma) * exp(-j * j / (2 * sigma * sigma));
				//	gaussian_values[j] = value;
				//	sample_offsets[j] = vec4((float)j, (float)j, 0.0f, 0.0f) / vec4(viewport_size, 1, 1);
				//}

				//float summ = std::accumulate(gaussian_values.begin(), gaussian_values.end(), 0.0f);
				//summ = std::accumulate(gaussian_values.begin() + 1, gaussian_values.end(), summ);
				//for (auto& value : gaussian_values)
				//	value /= summ;
				//
				//vec2 blur_direction(blur_direction_sigma.x > 0.1f ? (float)radius : 0.0f, blur_direction_sigma.y > 0.1f ? (float)radius : 0.0f);

				//ConstantBindings constants;
				//constants.AddFloat2Binding(&blur_direction, "blur_direction");
				//constants.AddDataBinding(sample_offsets.data(), sample_offsets.size() * sizeof(float4), "blur_offsets");
				//constants.AddDataBinding(gaussian_values.data(), gaussian_values.size() * sizeof(float), "blur_weights");

				//ResourceBindings resource_bindings;
				//resource_bindings.AddTextureBinding("src_texture", src_target_node.resource->GetAttachment()->GetTexture().get());
				//const auto* descriptor_set_layout = shader_blur->GetDescriptorSetLayout(0);
				//const DescriptorSetBindings bindings(resource_bindings, *descriptor_set_layout);

				//auto command_buffer = state.GetCurrentCommandBuffer()->GetCommandBuffer();

				//state.SetGlobalBindings(resource_bindings, constants);

				//state.SetRenderMode(mode);
				//state.SetShader(*shader_blur);
				//state.SetVertexLayout(full_screen_quad_mesh->GetVertexLayout());
				//state.UpdateState();
				//state.SetDescriptorSetBindings(bindings, constants);

				//state.Draw(*full_screen_quad_mesh->vertexBuffer(), full_screen_quad_mesh->indexCount(), 0);
			});

		return blur_info.color_output;
	}

	DependencyNode* Blur::AddFullBlur(RenderGraph& graph, DependencyNode& src_target_node, ResourceWrapper& intermediate_target, ResourceWrapper& destination_target, float sigma)
	{
		auto intermediate = AddOneDirectionBlur(graph, src_target_node, intermediate_target, vec2(sigma, 0));
		auto result = AddOneDirectionBlur(graph, *intermediate, destination_target, vec2(0, sigma));
		return result;
	}

}