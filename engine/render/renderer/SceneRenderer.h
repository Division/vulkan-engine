#pragma once

#include "CommonIncludes.h"
#include "utils/Pool.h"
#include "render/renderer/IRenderer.h"
#include "render/shader/ShaderResource.h"
#include "render/shader/Shader.h"
#include "ecs/systems/CullingSystem.h"
#include "utils/EventDispatcher.h"
#include <magic_enum/magic_enum.hpp>

#include <rps/rps.h>

namespace Resources
{
	class TextureResource;
}

namespace Device
{
	class VulkanRenderTargetAttachment;
	class ShaderCache;
	class Texture;
}

class Scene;

namespace render {

	class SceneBuffers;
	struct DebugSettings;
	struct EnvironmentSettings;

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


	class RpsGraphHandle
	{
	public:
		RpsGraphHandle() = default;
		RpsGraphHandle(const RpsRenderGraphCreateInfo& info);
		RpsGraphHandle(RpsGraphHandle&& other);
		RpsGraphHandle& operator=(RpsGraphHandle&& other);

		RpsGraphHandle(const RpsGraphHandle&) = delete;
		RpsGraphHandle& operator=(const RpsGraphHandle&) = delete;

		RpsRenderGraph operator*() const { return handle; }

		RpsSubprogram GetMainEntry() const;

		~RpsGraphHandle();

	private:

		RpsRenderGraph handle = RPS_NULL_HANDLE;
	};


	class RpsSubprogramHandle
	{
	public:
		RpsSubprogramHandle() = default;
		RpsSubprogramHandle(const RpsProgramCreateInfo& info);
		RpsSubprogramHandle(RpsSubprogramHandle&& other);
		RpsSubprogramHandle& operator=(RpsSubprogramHandle&& other);

		RpsSubprogramHandle(const RpsSubprogramHandle&) = delete;
		RpsSubprogramHandle& operator=(const RpsSubprogramHandle&) = delete;
		~RpsSubprogramHandle();

		RpsSubprogram operator*() const { return handle; }

	private:

		RpsSubprogram handle = RPS_NULL_HANDLE;
	};


	class SceneRenderer : IRenderer
	{
	public:
		SceneRenderer(Scene& scene, Device::ShaderCache* shader_cache, DebugSettings* settings);
		~SceneRenderer();

		tl::expected<RpsRenderGraph, std::string> LoadGraph(const char* path, bool compile);
		void RenderSceneGraph(RpsRenderGraph graph, gsl::span<const RpsConstant> args, gsl::span<const RpsRuntimeResource const*> resources);
		SceneBuffers* GetSceneBuffers() const { return scene_buffers.get(); }
		Device::ShaderCache* GetShaderCache() const { return shader_cache; }
		auto* GetEnvironmentSettings() const { return environment_settings.get(); }
		void SetRadianceCubemap(Resources::Handle<Resources::TextureResource> cubemap);
		void SetIrradianceCubemap(Resources::Handle<Resources::TextureResource> cubemap);

		Device::Texture* GetBlankTexture() const;

		const RendererResources& GetRendererResources() const { return *renderer_resources; }

	private:
		void OnRecreateSwapchain(int32_t width, int32_t height);

	private:

		Scene& scene;

		std::unique_ptr<RendererResources> renderer_resources;

		Device::ShaderCache* shader_cache;
		std::unique_ptr<SceneBuffers> scene_buffers;

		std::unique_ptr<Device::VulkanRenderTargetAttachment> main_depth_attachment;
		std::unique_ptr<Device::VulkanRenderTargetAttachment> shadowmap_atlas_attachment;
		std::unique_ptr<Device::VulkanRenderTargetAttachment> shadowmap_attachment;
		std::unique_ptr<Device::VulkanRenderTargetAttachment> main_color_attachment[2];

		std::unique_ptr<EnvironmentSettings> environment_settings;
		DebugSettings* debug_settings;
		float time;
	};

}