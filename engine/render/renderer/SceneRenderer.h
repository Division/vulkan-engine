#pragma once

#include "CommonIncludes.h"
#include "utils/Pool.h"
#include "render/renderer/DrawCall.h"
#include "render/renderer/IRenderer.h"
#include "render/shader/ShaderResource.h"
//#include "render/buffer/UniformBuffer.h"

class Scene;
class IShadowCaster;

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
		template<typename T> class UniformBuffer;
	}

	namespace ECS
	{
		namespace systems
		{
			class RendererToROPSystem;
		}

		class EntityManager;
	}
	
}

namespace core { namespace render {

	class SceneBuffers;
	class LightGrid;
	class ShadowMap;

	namespace graph
	{
		class RenderGraph;
	}


	class SceneRenderer : IRenderer
	{
	public:
		static int32_t ShadowAtlasSize();

		SceneRenderer(ShaderCache* shader_cache);
		~SceneRenderer();

		void RenderScene(Scene* scene);
		void AddRenderOperation(core::Device::RenderOperation& rop, RenderQueue queue) override;

	private:
		DrawCall* GetDrawCall(RenderOperation& rop, bool depth_only = false, uint32_t camera_index = 0);
		void ReleaseDrawCalls();
		void SetupShaderBindings(RenderOperation& rop, ShaderProgram& shader, ShaderBindings& bindings, uint32_t camera_index);
		std::tuple<vk::Buffer, size_t, size_t> GetBufferFromROP(RenderOperation& rop, ShaderBufferName buffer_name, uint32_t camera_index);
		void OnRecreateSwapchain(int32_t width, int32_t height);
		Texture* GetTextureFromROP(RenderOperation& rop, ShaderTextureName texture_name);
		void AddROPsFromECS(ECS::EntityManager* manager);

	private:
		std::unique_ptr<VulkanRenderPass> temp_pass;

		ShaderCache* shader_cache;
		std::unique_ptr<SceneBuffers> scene_buffers;
		std::unique_ptr<LightGrid> light_grid;
		std::unique_ptr<ShadowMap> shadow_map;
		std::unique_ptr<graph::RenderGraph> render_graph;
		core::utils::Pool<DrawCall> draw_call_pool;
		std::vector<std::unique_ptr<DrawCall>> used_draw_calls;
		
		ShaderProgram* compute_program;
		std::unique_ptr<ShaderBindings> compute_bindings;
		std::unique_ptr<UniformBuffer<unsigned char>> compute_buffer;

		std::unique_ptr<VulkanRenderTargetAttachment> main_depth_attachment;
		std::unique_ptr<VulkanRenderTargetAttachment> shadowmap_atlas_attachment;
		std::unique_ptr<core::ECS::systems::RendererToROPSystem> renderer_to_rop_system;
		std::array<std::vector<DrawCall*>, (size_t)RenderQueue::Count> render_queues;
		uint32_t depth_only_fragment_shader_hash;
		std::vector<std::pair<IShadowCaster*, std::vector<DrawCall*>>> shadow_casters;
	};

} }