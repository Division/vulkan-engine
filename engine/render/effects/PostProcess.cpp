#include "CommonIncludes.h"
#include "PostProcess.h"
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

namespace core::render::effects 

{
	using namespace core::render::graph;
	using namespace core::ECS;
	PostProcess::PostProcess(ShaderCache& shader_cache, EnvironmentSettings& environment_settings)
		: environment_settings(environment_settings)
	{
		shader = shader_cache.GetShaderProgram(L"shaders/postprocess/postprocess.vert", L"shaders/postprocess/postprocess.frag");
		full_screen_quad_mesh = std::make_unique<Mesh>(false);
		MeshGeneration::generateFullScreenQuad(full_screen_quad_mesh.get());
		full_screen_quad_mesh->createBuffer();

		src_texture_address = shader->GetBindingAddress("src_texture");
		hdr_buffer_address = shader->GetBindingAddress("hdr_data");
	}

	void PostProcess::PrepareRendering(RenderGraph& graph)
	{
		attachment_wrappers[0] = graph.RegisterAttachment(*attachments[0]);
		attachment_wrappers[1] = graph.RegisterAttachment(*attachments[1]);
	}

	graph::DependencyNode* PostProcess::AddPostProcess(RenderGraph& graph, DependencyNode& node, ResourceWrapper& destination_target, ResourceWrapper& hdr_buffer)
	{
		auto* destination_render_target = destination_target.GetAttachment();

		struct PassInfo
		{
			graph::DependencyNode* color_output = nullptr;
		};

		auto post_process_info = graph.AddPass<PassInfo>("postprocess", [&](graph::IRenderPassBuilder& builder)
			{
				PassInfo result;
				builder.AddInput(node);
				result.color_output = builder.AddOutput(destination_target);
				return result;
			}, [&](VulkanRenderState& state)
			{
				RenderMode mode;
				mode.SetDepthWriteEnabled(false);
				mode.SetDepthTestEnabled(false);
				mode.SetPolygonMode(PolygonMode::Fill);
				mode.SetPrimitiveTopology(PrimitiveTopology::TriangleList);

				ShaderBindings bindings;
				bindings.AddTextureBindingSafe(src_texture_address, node.resource->GetAttachment()->GetTexture().get());

				auto* buffer = hdr_buffer.GetBuffer();
				bindings.AddBufferBindingSafe(hdr_buffer_address, 0, buffer->Size(), buffer->Buffer());
				auto* descriptor_cache = Engine::GetVulkanContext()->GetDescriptorCache();
				auto vk_descriptor_set = descriptor_cache->GetDescriptorSet(bindings, *shader->GetDescriptorSet(0));
				auto command_buffer = state.GetCurrentCommandBuffer()->GetCommandBuffer();

				state.SetRenderMode(mode);
				state.SetShader(*shader);
				state.SetVertexLayout(full_screen_quad_mesh->GetVertexLayout());
				state.UpdateState();

				auto pipeline = state.GetCurrentPipeline();

				command_buffer.pushConstants(pipeline->GetPipelineLayout(), vk::ShaderStageFlagBits::eFragment, 0, sizeof(float), &environment_settings.exposure);
				command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline->GetPipelineLayout(), 0, 1u, &vk_descriptor_set, 0, nullptr);
				state.Draw(*full_screen_quad_mesh->vertexBuffer(), full_screen_quad_mesh->indexCount(), 0);
			});

		current_target = (current_target + 1) % attachments.size();

		return post_process_info.color_output;
	}

	void PostProcess::OnRecreateSwapchain(int32_t width, int32_t height)
	{
		attachments[0] = std::make_unique<VulkanRenderTargetAttachment>(VulkanRenderTargetAttachment::Type::Color, width, height, Format::R16G16B16A16_float);
		attachments[1] = std::make_unique<VulkanRenderTargetAttachment>(VulkanRenderTargetAttachment::Type::Color, width, height, Format::R16G16B16A16_float);
	}

}