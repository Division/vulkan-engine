#include "CommonIncludes.h"
#include "PostProcess.h"
#include "render/renderer/RenderGraph.h"
#include "render/device/VulkanRenderTarget.h"
#include "render/device/VulkanRenderState.h"
#include "render/shader/Shader.h"
#include "render/texture/Texture.h"
#include "render/mesh/Mesh.h"
#include "render/shader/ShaderCache.h"
#include "utils/MeshGeneration.h"
#include "ecs/components/DrawCall.h"

namespace core::render::effects 

{
	using namespace core::render::graph;
	using namespace core::ECS;
	PostProcess::PostProcess(ShaderCache& shader_cache)
	{
		shader = shader_cache.GetShaderProgram(L"shaders/postprocess/postprocess.vert", L"shaders/postprocess/postprocess.frag");
		full_screen_quad_mesh = std::make_unique<Mesh>(false);
		MeshGeneration::generateFullScreenQuad(full_screen_quad_mesh.get());
		full_screen_quad_mesh->createBuffer();

		src_texture_address = shader->GetBindingAddress("src_texture");
	}

	void PostProcess::PrepareRendering(RenderGraph& graph)
	{
		attachment_wrappers[0] = graph.RegisterAttachment(*attachments[0]);
		attachment_wrappers[1] = graph.RegisterAttachment(*attachments[1]);
	}

	graph::DependencyNode* PostProcess::AddPostProcess(RenderGraph& graph, DependencyNode& node, ResourceWrapper& destination_target)
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

				//ShaderBindings bindings;
				//bindings.AddTextureBindingSafe(src_texture_address, node.resource->GetAttachment()->GetTexture().get());

				state.SetRenderMode(mode);
				components::DrawCall draw_call;
				draw_call.shader = shader;
				draw_call.mesh = full_screen_quad_mesh.get();
				draw_call.descriptor_set = nullptr;
				state.RenderDrawCall(&draw_call, false);
			});

		current_target = (current_target + 1) % attachments.size();

		return post_process_info.color_output;
	}

	void PostProcess::OnRecreateSwapchain(int32_t width, int32_t height)
	{
		attachments[0] = std::make_unique<VulkanRenderTargetAttachment>(VulkanRenderTargetAttachment::Type::Color, width, height, Format::R8G8B8A8_unorm);
		attachments[1] = std::make_unique<VulkanRenderTargetAttachment>(VulkanRenderTargetAttachment::Type::Color, width, height, Format::R8G8B8A8_unorm);
	}

}