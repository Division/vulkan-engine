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
	class DescriptorSetBindings;
	class ResourceBindings;
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

	namespace components
	{
		struct DirectionalLight;
		struct LightBase;
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

	struct ShadowCasterData
	{
		ECS::components::LightBase* light;
		ECS::systems::CullingSystem culling;
		size_t camera_offset;
	};

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
		const Device::ResourceBindings& GetGlobalResourceBindings();
		Device::ShaderCache* GetShaderCache() const { return shader_cache; }
		auto* GetEnvironmentSettings() const { return environment_settings.get(); }
		LightGrid& GetLightGrid() { return *light_grid; }
		void SetRadianceCubemap(Resources::Handle<Resources::TextureResource> cubemap);
		void SetIrradianceCubemap(Resources::Handle<Resources::TextureResource> cubemap);

		Device::Texture* GetBlankTexture() const;

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
		std::unique_ptr<ECS::components::DirectionalLight> directional_light;
		std::unique_ptr<SceneBuffers> scene_buffers;
		std::unique_ptr<LightGrid> light_grid;
		std::unique_ptr<ShadowMap> shadow_map;
		std::unique_ptr<Device::ResourceBindings> global_resource_bindings;
		bool global_bindings_dirty = true;
		uint32_t global_shader_binding_camera_index;
		std::mutex global_bindings_mutex;

		std::unique_ptr<graph::RenderGraph> render_graph;
		
		Device::ShaderProgram* global_bindings_program;
		Device::ShaderProgram* compute_program;
		std::unique_ptr<Device::DescriptorSetBindings> compute_bindings;
		std::unique_ptr<Device::DynamicBuffer<unsigned char>> compute_buffer;

		std::unique_ptr<Device::VulkanRenderTargetAttachment> main_depth_attachment;
		std::unique_ptr<Device::VulkanRenderTargetAttachment> shadowmap_atlas_attachment;
		std::unique_ptr<Device::VulkanRenderTargetAttachment> shadowmap_attachment;
		std::unique_ptr<Device::VulkanRenderTargetAttachment> main_color_attachment;

		uint32_t depth_only_fragment_shader_hash;

		std::vector<ShadowCasterData> shadow_casters;

		std::unique_ptr<EnvironmentSettings> environment_settings;
		DebugSettings* debug_settings;
		std::unique_ptr<effects::Skybox> skybox;
		std::unique_ptr<effects::PostProcess> post_process;
	};

}