#pragma once

#include "CommonIncludes.h"
#include "utils/Pool.h"
#include "render/renderer/IRenderer.h"
#include "render/shader/ShaderResource.h"
#include "render/shader/Shader.h"
#include "ecs/systems/CullingSystem.h"

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
			class UploadDrawCallsSystem;
			class CreateDrawCallsSystem;
		}

		class EntityManager;
	}
	
}

class Scene;

namespace core { namespace render {

	class SceneBuffers;
	class LightGrid;
	class ShadowMap;
	class DrawCallManager;

	namespace graph
	{
		class RenderGraph;
	}


	class SceneRenderer : IRenderer
	{
	public:
		static int32_t ShadowAtlasSize();

		SceneRenderer(Scene& scene, ShaderCache* shader_cache);
		~SceneRenderer();

		void RenderScene();
		void AddRenderOperation(core::Device::RenderOperation& rop, RenderQueue queue) override;
		SceneBuffers* GetSceneBuffers() const { return scene_buffers.get(); }
		std::tuple<vk::Buffer, size_t> SceneRenderer::GetBufferData(ShaderBufferName buffer_name);
		Texture* SceneRenderer::GetTexture(ShaderTextureName texture_name, const Material& material);
		ShaderCache* GetShaderCache() const { return shader_cache; }
		void SetupShaderBindings(const Material& material, const ShaderProgram::DescriptorSet& descriptor_set, ShaderBindings& bindings);

	private:
		void CreateDrawCalls();
		void UploadDrawCalls();
		void UpdateGlobalBindings();
		void SetupShaderBindings(RenderOperation& rop, ShaderProgram& shader, ShaderBindings& bindings, uint32_t camera_index); // TODO: remove
		void OnRecreateSwapchain(int32_t width, int32_t height);
		std::tuple<vk::Buffer, size_t, size_t> GetBufferFromROP(RenderOperation& rop, ShaderBufferName buffer_name, uint32_t camera_index);
		void AddROPsFromECS(ECS::EntityManager* manager);

	private:
		std::unique_ptr<VulkanRenderPass> temp_pass;

		Scene& scene;

		std::unique_ptr<DrawCallManager> draw_call_manager;
		std::unique_ptr<core::ECS::systems::CreateDrawCallsSystem> create_draw_calls_system;
		std::unique_ptr<core::ECS::systems::UploadDrawCallsSystem> upload_draw_calls_system;
		std::unique_ptr<core::ECS::systems::RendererToROPSystem> renderer_to_rop_system;

		ShaderCache* shader_cache;
		std::unique_ptr<SceneBuffers> scene_buffers;
		std::unique_ptr<LightGrid> light_grid;
		std::unique_ptr<ShadowMap> shadow_map;
		std::unique_ptr<ShaderBindings> global_shader_bindings;
		uint32_t global_shader_binding_camera_index;

		std::unique_ptr<graph::RenderGraph> render_graph;
		
		ShaderProgram* compute_program;
		std::unique_ptr<ShaderBindings> compute_bindings;
		std::unique_ptr<UniformBuffer<unsigned char>> compute_buffer;

		std::unique_ptr<VulkanRenderTargetAttachment> main_depth_attachment;
		std::unique_ptr<VulkanRenderTargetAttachment> shadowmap_atlas_attachment;

		uint32_t depth_only_fragment_shader_hash;
		std::vector<std::pair<IShadowCaster*, core::ECS::systems::CullingSystem>> shadow_casters;
	};

} }