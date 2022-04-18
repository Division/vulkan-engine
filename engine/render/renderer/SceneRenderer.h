#pragma once

#include "CommonIncludes.h"
#include "utils/Pool.h"
#include "render/renderer/IRenderer.h"
#include "render/shader/ShaderResource.h"
#include "render/shader/Shader.h"
#include "ecs/systems/CullingSystem.h"
#include "utils/EventDispatcher.h"
#include <magic_enum/magic_enum.hpp>

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
	class ConstantBindings;
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
	class BitonicSort;
	class Bloom;
	class Blur;
	class ConstantBindingStorage;

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
	namespace GPUParticles
	{
		class GPUParticles;
	}

	namespace graph
	{
		class RenderGraph;
		class DependencyNode;
	}

	struct RenderCallbackData
	{
		SceneRenderer& scene_renderer;
		const Device::ConstantBindings& global_constant_bindings;
		const Device::ResourceBindings& global_resource_bindings;
	};

	struct RendererResources
	{
		Device::Handle<Device::Texture> blank_texture;
		Device::Handle<Device::Texture> blank_cube_texture;
		Resources::Handle<Resources::TextureResource> environment_cubemap;
		Resources::Handle<Resources::TextureResource> radiance_cubemap;
		Resources::Handle<Resources::TextureResource> irradiance_cubemap;
		Resources::Handle<Resources::TextureResource> brdf_lut;
		std::unique_ptr<Mesh> full_screen_quad_mesh;
		Common::Handle<Mesh> particle_quad_mesh;

	private:
		friend class SceneRenderer;
		RendererResources();
	};

	class SceneRenderer : IRenderer
	{
	public:
		static int32_t ShadowAtlasSize();

		enum class RenderDependencyType
		{
			DepthPrepass,
			Main
		};

		typedef utils::EventDispatcher<const RenderCallbackData&, graph::RenderGraph&> RenderDispatcher;

		SceneRenderer(Scene& scene, Device::ShaderCache* shader_cache, DebugSettings* settings);
		~SceneRenderer();

		void RenderScene(float dt);
		SceneBuffers* GetSceneBuffers() const { return scene_buffers.get(); }
		DrawCallManager* GetDrawCallManager() const { return draw_call_manager.get(); }
		const Device::ResourceBindings& GetGlobalResourceBindings();
		Device::ShaderCache* GetShaderCache() const { return shader_cache; }
		auto* GetEnvironmentSettings() const { return environment_settings.get(); }
		LightGrid& GetLightGrid() { return *light_grid; }
		void SetRadianceCubemap(Resources::Handle<Resources::TextureResource> cubemap);
		void SetIrradianceCubemap(Resources::Handle<Resources::TextureResource> cubemap);
		ConstantBindingStorage& GetConstantStorage() const { return *constant_storage; }

		Device::Texture* GetBlankTexture() const;

		auto AddRenderCallback(RenderDispatcher::Callback callback) { return render_dispatcher.AddCallback(callback); }
		void AddUserRenderDependency(RenderDependencyType type, graph::DependencyNode& node) { user_dependencies[(uint32_t)type].push_back({ &node }); }

		const RendererResources& GetRendererResources() const { return *renderer_resources; }
		const GPUParticles::GPUParticles& GetGPUParticles() const { return *gpu_particles; }

	private:
		void CreateDrawCalls();
		void UploadDrawCalls();
		void UpdateGlobalBindings();
		void OnRecreateSwapchain(int32_t width, int32_t height);

	private:
		struct UserRenderDependency
		{
			graph::DependencyNode* node;
		};

		std::array<std::vector<UserRenderDependency>, magic_enum::enum_count<RenderDependencyType>()> user_dependencies;
		std::unique_ptr<Device::VulkanRenderPass> temp_pass;
		utils::EventDispatcher<const RenderCallbackData&, graph::RenderGraph&> render_dispatcher;

		Scene& scene;

		std::unique_ptr<DrawCallManager> draw_call_manager;
		std::unique_ptr<ECS::systems::CreateDrawCallsSystem> create_draw_calls_system;
		std::unique_ptr<ECS::systems::UploadDrawCallsSystem> upload_draw_calls_system;
		std::unique_ptr<ECS::systems::UploadSkinningSystem> upload_skinning_system;

		std::unique_ptr<RendererResources> renderer_resources;

		Device::ShaderCache* shader_cache;
		std::unique_ptr<ECS::components::DirectionalLight> directional_light;
		std::unique_ptr<SceneBuffers> scene_buffers;
		std::unique_ptr<LightGrid> light_grid;
		std::unique_ptr<ShadowMap> shadow_map;
		std::unique_ptr<Device::ResourceBindings> global_resource_bindings;
		std::unique_ptr<Device::ConstantBindings> global_constant_bindings;
		bool global_bindings_dirty = true;
		uint32_t global_shader_binding_camera_index;
		std::mutex global_bindings_mutex;

		std::unique_ptr<graph::RenderGraph> render_graph;
		
		std::unique_ptr<Device::VulkanRenderTargetAttachment> main_depth_attachment;
		std::unique_ptr<Device::VulkanRenderTargetAttachment> shadowmap_atlas_attachment;
		std::unique_ptr<Device::VulkanRenderTargetAttachment> shadowmap_attachment;
		std::unique_ptr<Device::VulkanRenderTargetAttachment> main_color_attachment[2];

		std::vector<ShadowCasterData> shadow_casters;

		std::unique_ptr<EnvironmentSettings> environment_settings;
		std::unique_ptr<ConstantBindingStorage> constant_storage;
		DebugSettings* debug_settings;
		std::unique_ptr<effects::Skybox> skybox;
		std::unique_ptr<effects::PostProcess> post_process;
		std::unique_ptr<BitonicSort> bitonic_sort;
		std::unique_ptr<GPUParticles::GPUParticles> gpu_particles;
		std::unique_ptr<Bloom> bloom;
		std::unique_ptr<Blur> blur;
		float time;
	};

}