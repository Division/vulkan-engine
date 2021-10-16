#include "SceneRenderer.h"
#include "ecs/ECS.h"
#include "ecs/components/MultiMeshRenderer.h"
#include "ecs/components/Transform.h"
#include "ecs/components/Light.h"
#include "ecs/systems/RendererSystem.h"
#include "ecs/systems/UpdateDrawCallsSystem.h"
#include "scene/Scene.h"
#include "render/device/VulkanUtils.h"
#include "CommonIncludes.h"
#include "Engine.h"
#include "render/device/VulkanContext.h"
#include "render/buffer/VulkanBuffer.h"
#include "render/device/VulkanUploader.h"
#include "render/device/VulkanUtils.h"
#include "render/device/VulkanRenderPass.h"
#include "render/device/VulkanSwapchain.h"
#include "render/device/VulkanPipeline.h"
#include "render/device/VulkanRenderState.h"
#include "render/device/VulkanRenderTarget.h"
#include "render/device/VkObjects.h"
#include "loader/TextureLoader.h"
#include "render/shading/LightGrid.h"
#include "render/texture/Texture.h"
#include "render/shading/ShadowMap.h"
#include "render/mesh/Mesh.h"
#include "render/shading/IShadowCaster.h"
#include "render/shader/ShaderCache.h"
#include "render/shader/ShaderResource.h"
#include "render/shader/ShaderBindings.h"
#include "render/shader/ShaderDefines.h"
#include "render/renderer/RenderGraph.h"
#include "render/material/Material.h"
#include "render/buffer/DynamicBuffer.h"
#include "render/debug/DebugDraw.h"
#include "render/debug/DebugUI.h"
#include "EnvironmentSettings.h"
#include "SceneBuffers.h"
#include "objects/Camera.h"
#include "render/effects/Skybox.h"
#include "render/effects/PostProcess.h"
#include "render/effects/Bloom.h"
#include "render/effects/Blur.h"
#include "resources/TextureResource.h"
#include "render/debug/DebugSettings.h"
#include "RenderModeUtils.h"

#include <functional>

using namespace Device;
using namespace Resources;

namespace render {

	using namespace ECS;
	using namespace profiler;

	static const int32_t shadow_atlas_size = 4096;

	int32_t SceneRenderer::ShadowAtlasSize()
	{
		return shadow_atlas_size;
	}

	void SceneRenderer::SetRadianceCubemap(Resources::Handle<Resources::TextureResource> cubemap)
	{
		radiance_cubemap = cubemap;
	}

	void SceneRenderer::SetIrradianceCubemap(Resources::Handle<Resources::TextureResource> cubemap)
	{
		irradiance_cubemap = cubemap;
	}

	SceneRenderer::~SceneRenderer() = default;

	SceneRenderer::SceneRenderer(Scene& scene, ShaderCache* shader_cache, DebugSettings* settings)
		: scene(scene)
		, shader_cache(shader_cache)
		, debug_settings(settings)
	{
		scene_buffers = std::make_unique<SceneBuffers>();
		directional_light = std::make_unique<components::DirectionalLight>();
		scene.GetEntityManager()->AddStaticComponent(directional_light.get());
		
		environment_settings = std::make_unique<EnvironmentSettings>();
		environment_settings->directional_light = directional_light.get();

		draw_call_manager = std::make_unique<DrawCallManager>(*this);
		create_draw_calls_system = std::make_unique<systems::CreateDrawCallsSystem>(*scene.GetEntityManager(), *draw_call_manager);
		upload_draw_calls_system = std::make_unique<systems::UploadDrawCallsSystem>(*draw_call_manager, *scene_buffers);
		upload_skinning_system = std::make_unique<systems::UploadSkinningSystem>(*draw_call_manager, *scene_buffers);

		light_grid = std::make_unique<LightGrid>();
		shadow_map = std::make_unique<ShadowMap>(ShadowAtlasSize(), ShadowAtlasSize());
		render_graph = std::make_unique<graph::RenderGraph>();
		depth_only_fragment_shader_hash = ShaderCache::GetShaderDataHash(ShaderProgramInfo::ShaderData(ShaderProgram::Stage::Fragment, L"shaders/noop.hlsl"));
		auto* context = Engine::GetVulkanContext();
		context->AddRecreateSwapchainCallback(std::bind(&SceneRenderer::OnRecreateSwapchain, this, std::placeholders::_1, std::placeholders::_2));
		shadowmap_atlas_attachment = std::make_unique<VulkanRenderTargetAttachment>("Shadowmap Atlas", VulkanRenderTargetAttachment::Type::Depth, ShadowAtlasSize(), ShadowAtlasSize(), Format::D24_unorm_S8_uint);
		shadowmap_attachment = std::make_unique<VulkanRenderTargetAttachment>("Shadowmap", VulkanRenderTargetAttachment::Type::Depth, ShadowAtlasSize(), ShadowAtlasSize(), Format::D24_unorm_S8_uint);

		global_resource_bindings = std::make_unique<Device::ResourceBindings>();
		global_constant_bindings = std::make_unique<Device::ConstantBindings>();

		brdf_lut = TextureResource::Linear(L"assets/Textures/LUT/ibl_brdf_lut.dds");

		compute_buffer = std::make_unique<DynamicBuffer<unsigned char>>(128 * 128 * sizeof(vec4), BufferType::Storage);

		uint32_t colors[16];
		for (auto& color : colors)
			color = 0xFFFF00FF;
		blank_texture = Device::Texture::Create(Device::TextureInitializer(4, 4, 4, colors, false));

		for (auto& color : colors)
			color = 0x000000;

		TextureInitializer cube_initializer(4, 4, 1, 6, Device::Format::R8G8B8A8_unorm, 1);
		cube_initializer.SetData(colors, sizeof(colors))
			.SetArray(false)
			.SetCube(true)
			.SetNumDimensions(2);

		for (int i = 0; i < 6; i++)
		{
			cube_initializer.AddCopy(
				vk::BufferImageCopy(0, 0, 0,
					vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, i, 1),
					vk::Offset3D(0, 0, 0),
					vk::Extent3D(4, 4, 1)
				));
		}

		blank_cube_texture = Device::Texture::Create(cube_initializer);
		
		skybox = std::make_unique<effects::Skybox>(*shader_cache);

		post_process = std::make_unique<effects::PostProcess>(*shader_cache, *environment_settings);
		blur = std::make_unique<Blur>(*shader_cache);
		bloom = std::make_unique<Bloom>(*shader_cache, *blur, *environment_settings);
	}

	void SceneRenderer::OnRecreateSwapchain(int32_t width, int32_t height)
	{
		render_graph->ClearCache();
		main_depth_attachment = std::make_unique<VulkanRenderTargetAttachment>("Main Depth", VulkanRenderTargetAttachment::Type::Depth, width, height, Format::D24_unorm_S8_uint);
		main_color_attachment[0] = std::make_unique<VulkanRenderTargetAttachment>("Main Color 0", VulkanRenderTargetAttachment::Type::Color, width, height, Format::R16G16B16A16_float);
		main_color_attachment[1] = std::make_unique<VulkanRenderTargetAttachment>("Main Color 1", VulkanRenderTargetAttachment::Type::Color, width, height, Format::R16G16B16A16_float);
		post_process->OnRecreateSwapchain(width, height);
		bloom->OnRecreateSwapchain(width, height);
	}

	void SceneRenderer::CreateDrawCalls()
	{
		auto* entity_manager = scene.GetEntityManager();
		auto list = entity_manager->GetChunkLists([=](ChunkList* chunk_list) {
			return chunk_list->HasComponent(GetComponentHash<components::MultiMeshRenderer>());
		});
		create_draw_calls_system->ProcessChunks(list);
	}

	void SceneRenderer::UploadDrawCalls()
	{
		upload_draw_calls_system->ProcessChunks(draw_call_manager->GetDrawCallChunks());
		upload_skinning_system->ProcessChunks(draw_call_manager->GetSkinningChunks());
	}

	static ShaderBufferStruct::Camera GetCameraData(ICameraParamsProvider* camera, uvec2 screen_size)
	{
		ShaderBufferStruct::Camera result;
		result.projectionMatrix = camera->cameraProjectionMatrix();
		result.projectionMatrix[1][1] *= -1;
		result.position = camera->cameraPosition();
		result.viewMatrix = camera->cameraViewMatrix();
		result.screenSize = screen_size;
		result.zMin = camera->cameraZMinMax().x;
		result.zMax = camera->cameraZMinMax().y;
		return result;
	}

	static ShaderBufferStruct::EnvironmentSettings GetEnvSettings(const EnvironmentSettings& settings)
	{
		ShaderBufferStruct::EnvironmentSettings result;

		result.direction_light_enabled = settings.directional_light && settings.directional_light->enabled;
		if (result.direction_light_enabled)
		{
			result.direction_light_color = settings.directional_light->color;
			result.direction_light_direction = settings.directional_light->transform.Forward();
			result.direction_light_projection_matrix = settings.directional_light->cameraViewProjectionMatrix();
			result.direction_light_cast_shadows = settings.directional_light->cast_shadows;
		}

		result.environment_brightness = settings.environment_brightness;
		result.exposure = settings.exposure;
		return result;
	}

	void SceneRenderer::RenderScene()
	{
		OPTICK_EVENT();

		draw_call_manager->Update();

		auto* entity_manager = scene.GetEntityManager();
		const bool direction_light_enabled = environment_settings->directional_light && environment_settings->directional_light->enabled;
		const bool directional_light_cast_shadows = direction_light_enabled && environment_settings->directional_light->cast_shadows;
		if (direction_light_enabled)
			environment_settings->directional_light->UpdateMatrices();

		light_grid->UpdateSlices(scene.GetCamera()->cameraProjectionMatrix());

		if (debug_settings && debug_settings->draw_clusters)
		{
			light_grid->DrawDebugClusters(debug_settings->cluster_matrix, vec4(1, 1, 1, 1));
		}

		CreateDrawCalls();
		draw_call_manager->ReleaseDrawCallLists();

		auto* context = Engine::GetVulkanContext();
		auto vk_device = context->GetDevice();
		auto vk_physical_device = context->GetPhysicalDevice();
		auto vk_swapchain = context->GetSwapchain();
		
		auto &visible_lights = scene.GetVisibleLights();

		
		// Shadow casters
		shadow_casters.clear();
		for (auto &light : visible_lights) {
			if (light.light->cast_shadows) {
				shadow_casters.push_back(
					{ light.light, systems::CullingSystem(*draw_call_manager, light.light->GetFrustum()), 0 }
				);
			}
		}

		/*auto &visibleProjectors = scene.visibleProjectors(scene.GetCamera());
		for (auto &projector : visibleProjectors) {
			if (projector->castShadows()) {
				shadow_casters.push_back(
					std::make_pair(static_cast<IShadowCaster*>(projector.get()), systems::CullingSystem(*draw_call_manager, *projector))
				);
			}
		}
		*/

		UploadDrawCalls();

		auto main_camera = GetCameraData(scene.GetCamera(), scene.GetCamera()->cameraViewSize());
		global_constant_bindings->AddDataBinding(&main_camera, sizeof(main_camera), Device::GetBufferMemberHash(BufferMemberName::Camera));

		auto environment_data = GetEnvSettings(*environment_settings);
		global_constant_bindings->AddDataBinding(&environment_data, sizeof(environment_data), Device::GetBufferMemberHash(BufferMemberName::Environment));

		auto draw_calls = draw_call_manager->GetDrawCallChunks();

		systems::CullingSystem main_camera_culling_system(*draw_call_manager, scene.GetCamera()->GetFrustum());
		main_camera_culling_system.ProcessChunks(draw_calls);

		Frustum directional_light_frustum;
		ShaderBufferStruct::Camera directional_light_camera = {};
		if (directional_light_cast_shadows)
			directional_light_frustum = environment_settings->directional_light->GetFrustum();

		systems::CullingSystem directional_light_culling_system(*draw_call_manager, directional_light_frustum);

		if (directional_light_cast_shadows)
		{
			directional_light_camera = GetCameraData(environment_settings->directional_light, uvec2(0, 0));
			directional_light_culling_system.ProcessChunks(draw_calls);
		}

		for (auto& shadow_caster : shadow_casters)
		{
			shadow_caster.culling.ProcessChunks(draw_calls); // Cull and fill draw call list
		}

		shadow_map->SetupShadowCasters(shadow_casters);

		// Light grid setup
		auto window_size = scene.GetCamera()->cameraViewSize();

		light_grid->appendLights(visible_lights, scene.GetCamera(), shadow_map->GetAtlasSize().x);
		//light_grid->appendProjectors(visibleProjectors, scene.GetCamera());
		light_grid->upload();

		UpdateGlobalBindings();

		// Render Graph
		render_graph->Clear();
		
		struct PassInfo
		{
			graph::DependencyNode* color_output = nullptr;
			graph::DependencyNode* depth_output = nullptr;
			graph::DependencyNode* compute_output = nullptr;
		};

		auto* swapchain = context->GetSwapchain();
		auto* main_color = render_graph->RegisterAttachment(*swapchain->GetColorAttachment(context->GetCurrentFrame()));
		std::array<graph::ResourceWrapper*, 2> main_offscreen_color;
		main_offscreen_color[0] = render_graph->RegisterAttachment(*main_color_attachment[0]);
		main_offscreen_color[1] = render_graph->RegisterAttachment(*main_color_attachment[1]);
		auto* main_depth = render_graph->RegisterAttachment(*main_depth_attachment);
		auto* shadow_map = render_graph->RegisterAttachment(*shadowmap_attachment);
		auto* shadow_map_atlas = render_graph->RegisterAttachment(*shadowmap_atlas_attachment);
		auto* compute_buffer_resource = render_graph->RegisterBuffer(*compute_buffer->GetBuffer());
		post_process->PrepareRendering(*render_graph);

		if (environment_settings->bloom_enabled)
			bloom->PrepareRendering(*render_graph);

		/*auto compute_pass_info = render_graph->AddPass<PassInfo>("compute pass", ProfilerName::PassCompute, [&](graph::IRenderPassBuilder& builder)
		{
			builder.SetCompute();

			PassInfo result;
			result.compute_output = builder.AddOutput(*compute_buffer_resource);
			return result;
		}, [&](VulkanRenderState& state)
		{
			state.RecordCompute(*compute_program, *compute_bindings, uvec3(128, 128, 1));
		});*/

		auto depth_pre_pass_info = render_graph->AddPass<PassInfo>("depth pre pass", ProfilerName::PassDepthPrepass, [&](graph::IRenderPassBuilder& builder)
		{
			PassInfo result;
			result.depth_output = builder.AddOutput(*main_depth)->Clear(1.0f);
			//builder.AddInput(*compute_pass_info.compute_output);
			return result;
		}, [&](VulkanRenderState& state)
		{
			RenderMode mode;
			mode.SetDepthWriteEnabled(true);
			mode.SetDepthTestEnabled(true);
			mode.SetDepthFunc(CompareOp::Less);

			state.SetRenderMode(mode);
			state.SetGlobalBindings(GetGlobalResourceBindings(), *global_constant_bindings);

			auto& render_queues = main_camera_culling_system.GetDrawCallList()->queues;
			for (auto* draw_call : render_queues[(size_t)RenderQueue::DepthOnly])
			{
				state.RenderDrawCall(draw_call, true);
			}
		});

		auto shadow_map_info = render_graph->AddPass<PassInfo>("shadow map", ProfilerName::PassShadowmap, [&](graph::IRenderPassBuilder& builder)
			{
				PassInfo result;
				result.depth_output = builder.AddOutput(*shadow_map)->Clear(1.0f);
				return result;
			}, [&](VulkanRenderState& state)
			{
				RenderMode mode;
				mode.SetDepthWriteEnabled(true);
				mode.SetDepthTestEnabled(true);
				mode.SetDepthFunc(CompareOp::Less);
				state.SetRenderMode(mode);

				const ResourceBindings& global_bindings = GetGlobalResourceBindings();
				auto constants = *global_constant_bindings;
				constants.AddDataBinding(&directional_light_camera, sizeof(directional_light_camera), Device::GetBufferMemberHash(BufferMemberName::Camera));

				state.SetGlobalBindings(global_bindings, constants);

				if (directional_light_cast_shadows)
				{
					auto& render_queues = directional_light_culling_system.GetDrawCallList()->queues;
					for (auto* draw_call : render_queues[(size_t)RenderQueue::DepthOnly])
					{
						state.RenderDrawCall(draw_call, true);
					}
				}

			});
		
		auto shadow_map_atlas_info = render_graph->AddPass<PassInfo>("shadow map atlas", ProfilerName::PassShadowmap, [&](graph::IRenderPassBuilder& builder)
		{
			PassInfo result;
			result.depth_output = builder.AddOutput(*shadow_map_atlas)->Clear(1.0f);
			return result;
		}, [&](VulkanRenderState& state)
		{
			RenderMode mode;
			mode.SetDepthWriteEnabled(true);
			mode.SetDepthTestEnabled(true);
			mode.SetDepthFunc(CompareOp::Less);
			state.SetRenderMode(mode);
			state.SetScissor(vec4(0, 0, ShadowAtlasSize(), ShadowAtlasSize()));
			const ResourceBindings& global_bindings = GetGlobalResourceBindings();
			ConstantBindings constants = *global_constant_bindings;

			for (auto& shadow_caster : shadow_casters)
			{
				auto camera_data = GetCameraData(shadow_caster.light, uvec2(0, 0));
				constants.AddDataBinding(&camera_data, sizeof(camera_data), Device::GetBufferMemberHash(BufferMemberName::Camera));
				state.RemoveGlobalBindings();
				state.SetGlobalBindings(global_bindings, constants);
				state.SetViewport(shadow_caster.light->viewport);
				auto& render_queues = shadow_caster.culling.GetDrawCallList()->queues;
				for (auto* draw_call : render_queues[(size_t)RenderQueue::DepthOnly])
				{
					state.RenderDrawCall(draw_call, true);
				}
			}
		});

		auto* debug_draw = Engine::Get()->GetDebugDraw();
		auto& debug_draw_calls_lines = debug_draw->GetLineDrawCalls();
		auto& debug_draw_calls_points = debug_draw->GetPointDrawCalls();

		auto main_pass_info = render_graph->AddPass<PassInfo>("main", ProfilerName::PassMain, [&](graph::IRenderPassBuilder& builder)
		{
			PassInfo result;
			builder.AddInput(*depth_pre_pass_info.depth_output, graph::InputUsage::DepthAttachment);
			builder.AddInput(*shadow_map_info.depth_output);
			builder.AddInput(*shadow_map_atlas_info.depth_output);
			result.color_output = builder.AddOutput(*main_offscreen_color[0])->Clear(vec4(0));
			return result;
		}, [&](VulkanRenderState& state)
		{
			state.SetGlobalBindings(GetGlobalResourceBindings(), *global_constant_bindings);
			skybox->Render(state);

			state.SetRenderMode(GetRenderModeForQueue(RenderQueue::Opaque));

			auto& render_queues = main_camera_culling_system.GetDrawCallList()->queues;
			for (auto* draw_call : render_queues[(size_t)RenderQueue::Opaque])
			{
				state.RenderDrawCall(draw_call, false);
			}

			state.SetRenderMode(GetRenderModeForQueue(RenderQueue::Translucent));
			for (auto* draw_call : render_queues[(size_t)RenderQueue::Translucent])
			{
				state.RenderDrawCall(draw_call, false);
			}

			// Debug
			auto debug_mode = GetRenderModeForQueue(RenderQueue::Debug);
			debug_mode.SetPolygonMode(PolygonMode::Line);
			debug_mode.SetPrimitiveTopology(PrimitiveTopology::LineList);
			state.SetRenderMode(debug_mode);

			for (auto& draw_call : debug_draw_calls_lines)
			{
				state.RenderDrawCall(&draw_call, false);
			}

			debug_mode.SetPolygonMode(PolygonMode::Point);
			debug_mode.SetPrimitiveTopology(PrimitiveTopology::PointList);
			state.SetRenderMode(debug_mode);

			for (auto& draw_call : debug_draw_calls_points)
			{
				state.RenderDrawCall(&draw_call, false);
			}

		});

		auto post_process_src = main_pass_info.color_output;
		graph::DependencyNode* bloom_texture = nullptr;

		if (environment_settings->bloom_enabled)
			bloom_texture = bloom->AddBloom(*render_graph, *post_process_src, *main_offscreen_color[1]);

		effects::PostProcess::Input post_process_input = {};
		post_process_input.src_texture = post_process_src;
		post_process_input.bloom_texture = bloom_texture;
		auto post_process_result = post_process->AddPostProcess(*render_graph, post_process_input, *main_color, *compute_buffer_resource, GetGlobalResourceBindings(), *global_constant_bindings);

		auto ui_pass_info = render_graph->AddPass<PassInfo>("Debug UI", ProfilerName::PassDebugUI, [&](graph::IRenderPassBuilder& builder)
			{
				PassInfo result;
				//builder.AddInput(*compute_pass_info.compute_output);
				result.color_output = builder.AddOutput(*main_color)->PresentSwapchain();
				return result;
			}, [&](VulkanRenderState& state)
			{
				DebugUI::Render(state);
			});

		render_graph->Prepare();
		render_graph->Render();

		scene_buffers->GetConstantBuffer()->Upload();
		scene_buffers->GetSkinningMatricesBuffer()->Upload();

	}

	Device::Texture* SceneRenderer::GetBlankTexture() const
	{
		return blank_texture.get();
	}

	const Device::ResourceBindings& SceneRenderer::GetGlobalResourceBindings()
	{
		std::scoped_lock lock(global_bindings_mutex);

		if (global_bindings_dirty)
		{
			global_resource_bindings->Clear();
			global_resource_bindings->AddTextureBinding(Device::GetShaderTextureNameHash(ShaderTextureName::ShadowMap), shadowmap_attachment->GetTexture().get());
			global_resource_bindings->AddTextureBinding(Device::GetShaderTextureNameHash(ShaderTextureName::ShadowMapAtlas), shadowmap_atlas_attachment->GetTexture().get());
			global_resource_bindings->AddTextureBinding(Device::GetShaderTextureNameHash(ShaderTextureName::EnvironmentCubemap), environment_cubemap ? environment_cubemap->Get().get() : blank_cube_texture.get());
			global_resource_bindings->AddTextureBinding(Device::GetShaderTextureNameHash(ShaderTextureName::RadianceCubemap), radiance_cubemap ? radiance_cubemap->Get().get() : blank_cube_texture.get());
			global_resource_bindings->AddTextureBinding(Device::GetShaderTextureNameHash(ShaderTextureName::IrradianceCubemap), irradiance_cubemap ? irradiance_cubemap->Get().get() : blank_cube_texture.get());
			global_resource_bindings->AddTextureBinding(Device::GetShaderTextureNameHash(ShaderTextureName::BrdfLUT), brdf_lut->Get().get());

			global_resource_bindings->AddBufferBinding(Device::GetShaderBufferNameHash(ShaderBufferName::Projector), light_grid->GetProjectorBuffer()->GetBuffer(), light_grid->GetProjectorBuffer()->GetSize());
			global_resource_bindings->AddBufferBinding(Device::GetShaderBufferNameHash(ShaderBufferName::Light), light_grid->GetLightsBuffer()->GetBuffer(), light_grid->GetLightsBuffer()->GetSize());
			global_resource_bindings->AddBufferBinding(Device::GetShaderBufferNameHash(ShaderBufferName::LightIndices), light_grid->GetLightIndexBuffer()->GetBuffer(), light_grid->GetLightIndexBuffer()->GetSize());
			global_resource_bindings->AddBufferBinding(Device::GetShaderBufferNameHash(ShaderBufferName::LightGrid), light_grid->GetLightGridBuffer()->GetBuffer(), light_grid->GetLightGridBuffer()->GetSize());

			global_resource_bindings->AddBufferBinding(Device::GetShaderBufferNameHash(ShaderBufferName::SkinningMatrices), scene_buffers->GetSkinningMatricesBuffer()->GetBuffer(), scene_buffers->GetSkinningMatricesBuffer()->GetSize());

			global_bindings_dirty = false;
		}

		return *global_resource_bindings;
	}

	void SceneRenderer::UpdateGlobalBindings()
	{
		global_bindings_dirty = true;
	}

}