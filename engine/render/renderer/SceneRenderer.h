#pragma once

#include "CommonIncludes.h"
#include "utils/Pool.h"
#include "render/renderer/IRenderer.h"
#include "render/shader/ShaderResource.h"
#include "render/shader/Shader.h"
#include "ecs/systems/CullingSystem.h"

class Scene;
class IShadowCaster;

namespace Resources
{
	class TextureResource;
}

namespace Device
{
	class VulkanRenderTargetAttachment;
	class VulkanRenderPass;
	class ShaderProgram;
	class ShaderBindings;
	class ShaderCache;
	class Texture;
	template<typename T> class DynamicBuffer;
}

namespace ECS
{
	namespace systems
	{
		class UploadDrawCallsSystem;
		class CreateDrawCallsSystem;
		class UploadSkinningSystem;
	}

	class EntityManager;
}
	
class Scene;

namespace render {

	class SceneBuffers;
	class LightGrid;
	class ShadowMap;
	class DrawCallManager;
	struct DebugSettings;
	struct EnvironmentSettings;

	namespace effects
	{
		class Skybox;
		class PostProcess;
	}

	namespace graph
	{
		class RenderGraph;
	}


	class SceneRenderer : IRenderer
	{
	public:
		static int32_t ShadowAtlasSize();

		SceneRenderer(Scene& scene, Device::ShaderCache* shader_cache, DebugSettings* settings);
		~SceneRenderer();

		void RenderScene();
		SceneBuffers* GetSceneBuffers() const { return scene_buffers.get(); }
		std::tuple<vk::Buffer, size_t> SceneRenderer::GetBufferData(ShaderBufferName buffer_name);
		Device::Texture* SceneRenderer::GetTexture(ShaderTextureName texture_name, const Material& material);
		Device::ShaderCache* GetShaderCache() const { return shader_cache; }
		void SetupShaderBindings(const Material& material, const Device::ShaderProgram::DescriptorSet& descriptor_set, Device::ShaderBindings& bindings);
		auto* GetEnvironmentSettings() const { return environment_settings.get(); }
		LightGrid& GetLightGrid() { return *light_grid; }
		void SetRadianceCubemap(Resources::Handle<Resources::TextureResource> cubemap);
		void SetIrradianceCubemap(Resources::Handle<Resources::TextureResource> cubemap);

	private:
		void CreateDrawCalls();
		void UploadDrawCalls();
		void UpdateGlobalBindings();
		void OnRecreateSwapchain(int32_t width, int32_t height);

	private:
		std::unique_ptr<Device::VulkanRenderPass> temp_pass;

		Scene& scene;

		std::unique_ptr<DrawCallManager> draw_call_manager;
		std::unique_ptr<ECS::systems::CreateDrawCallsSystem> create_draw_calls_system;
		std::unique_ptr<ECS::systems::UploadDrawCallsSystem> upload_draw_calls_system;
		std::unique_ptr<ECS::systems::UploadSkinningSystem> upload_skinning_system;
		Device::Handle<Device::Texture> blank_texture;
		Device::Handle<Device::Texture> blank_cube_texture;
		Resources::Handle<Resources::TextureResource> environment_cubemap;
		Resources::Handle<Resources::TextureResource> radiance_cubemap;
		Resources::Handle<Resources::TextureResource> irradiance_cubemap;
		Resources::Handle<Resources::TextureResource> brdf_lut;

		Device::ShaderCache* shader_cache;
		std::unique_ptr<SceneBuffers> scene_buffers;
		std::unique_ptr<LightGrid> light_grid;
		std::unique_ptr<ShadowMap> shadow_map;
		std::unique_ptr<Device::ShaderBindings> global_shader_bindings;
		uint32_t global_shader_binding_camera_index;

		std::unique_ptr<graph::RenderGraph> render_graph;
		
		Device::ShaderProgram* global_bindings_program;
		Device::ShaderProgram* compute_program;
		std::unique_ptr<Device::ShaderBindings> compute_bindings;
		std::unique_ptr<Device::DynamicBuffer<unsigned char>> compute_buffer;

		std::unique_ptr<Device::VulkanRenderTargetAttachment> main_depth_attachment;
		std::unique_ptr<Device::VulkanRenderTargetAttachment> shadowmap_atlas_attachment;
		std::unique_ptr<Device::VulkanRenderTargetAttachment> main_color_attachment;

		uint32_t depth_only_fragment_shader_hash;
		std::vector<std::pair<IShadowCaster*, ECS::systems::CullingSystem>> shadow_casters;

		std::unique_ptr<EnvironmentSettings> environment_settings;
		DebugSettings* debug_settings;
		std::unique_ptr<effects::Skybox> skybox;
		std::unique_ptr<effects::PostProcess> post_process;
	};

}