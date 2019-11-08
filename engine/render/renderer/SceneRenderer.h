#pragma once

#include "CommonIncludes.h"
#include "utils/Pool.h"
#include "render/renderer/DrawCall.h"
#include "render/renderer/IRenderer.h"
#include "render/shader/ShaderResource.h"

class Scene;

namespace core
{
	namespace Device
	{
		class VulkanRenderTargetAttachment;
		class VulkanRenderPass;
		class ShaderProgram;
		class ShaderBindings;
		class ShaderCache;
		struct RenderOperation;
	}
}

namespace core { namespace render {

	class SceneBuffers;
	class LightGrid;

	namespace graph
	{
		class RenderGraph;
	}

	class SceneRenderer : IRenderer
	{
	public:
		SceneRenderer(ShaderCache* shader_cache);
		~SceneRenderer();

		void RenderScene(Scene* scene);
		void AddRenderOperation(core::Device::RenderOperation& rop, RenderQueue queue) override;

	private:
		DrawCall* GetDrawCall(RenderOperation& rop, bool depth_only = false);
		void ReleaseDrawCalls();
		void SetupShaderBindings(RenderOperation& rop, ShaderProgram& shader, ShaderBindings& bindings);
		std::tuple<vk::Buffer, size_t, size_t> GetBufferFromROP(RenderOperation& rop, ShaderBufferName buffer_name);

	private:
		std::unique_ptr<VulkanRenderPass> temp_pass;

		ShaderCache* shader_cache;
		std::unique_ptr<SceneBuffers> scene_buffers;
		std::unique_ptr<LightGrid> light_grid;
		std::unique_ptr<graph::RenderGraph> render_graph;
		core::utils::Pool<DrawCall> draw_call_pool;
		std::vector<std::unique_ptr<DrawCall>> used_draw_calls;
		std::unique_ptr<VulkanRenderTargetAttachment> temp_color_attachment;
		std::unordered_map<RenderOperation*, vk::DescriptorBufferInfo> rop_transform_cache; // cleared every frame. Allows reusing same object transform buffer in multiple draw calls.
		std::array<std::vector<DrawCall*>, (size_t)RenderQueue::Count> render_queues;
		uint32_t depth_only_fragment_shader_hash;
	};

} }