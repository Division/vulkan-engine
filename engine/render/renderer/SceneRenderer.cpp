#include "SceneRenderer.h"
#include "ecs/ECS.h"
#include "ecs/components/MeshRenderer.h"
#include "ecs/components/Transform.h"
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
#include "resources/ModelBundle.h"
#include "loader/ModelLoader.h"
#include "loader/TextureLoader.h"
#include "render/shading/LightGrid.h"
#include "render/texture/Texture.h"
#include "render/shading/ShadowMap.h"
#include "render/mesh/Mesh.h"
#include "render/shading/IShadowCaster.h"
#include "render/shader/ShaderCache.h"
#include "render/shader/ShaderBindings.h"
#include "render/shader/ShaderDefines.h"
#include "render/renderer/RenderGraph.h"
#include "render/material/Material.h"
#include "render/buffer/DynamicBuffer.h"
#include "render/debug/DebugDraw.h"
#include "render/debug/DebugUI.h"
#include "objects/LightObject.h"
#include "objects/Projector.h"
#include "SceneBuffers.h"
#include "objects/Camera.h"

#include <functional>

using namespace core;
using namespace core::Device;

namespace core { namespace render {

	using namespace ECS;

	static const int32_t shadow_atlas_size = 4096;

	int32_t SceneRenderer::ShadowAtlasSize()
	{
		return shadow_atlas_size;
	}

	SceneRenderer::~SceneRenderer() = default;

	SceneRenderer::SceneRenderer(Scene& scene, ShaderCache* shader_cache)
		: scene(scene)
		, shader_cache(shader_cache)
	{
		scene_buffers = std::make_unique<SceneBuffers>();

		draw_call_manager = std::make_unique<DrawCallManager>(*this);
		create_draw_calls_system = std::make_unique<systems::CreateDrawCallsSystem>(*scene.GetEntityManager(), *draw_call_manager);
		upload_draw_calls_system = std::make_unique<systems::UploadDrawCallsSystem>(*draw_call_manager, *scene_buffers);

		light_grid = std::make_unique<LightGrid>();
		shadow_map = std::make_unique<ShadowMap>(ShadowAtlasSize(), ShadowAtlasSize());
		render_graph = std::make_unique<graph::RenderGraph>();
		depth_only_fragment_shader_hash = ShaderCache::GetShaderPathHash(L"shaders/noop.frag");
		auto* context = Engine::GetVulkanContext();
		context->AddRecreateSwapchainCallback(std::bind(&SceneRenderer::OnRecreateSwapchain, this, std::placeholders::_1, std::placeholders::_2));
		shadowmap_atlas_attachment = std::make_unique<VulkanRenderTargetAttachment>(VulkanRenderTargetAttachment::Type::Depth, ShadowAtlasSize(), ShadowAtlasSize(), Format::D24_unorm_S8_uint);

		compute_buffer = std::make_unique<DynamicBuffer<unsigned char>>(128 * 128 * sizeof(vec4), BufferType::Storage);
		compute_program = shader_cache->GetShaderProgram(0, 0, ShaderCache::GetShaderPathHash(L"shaders/test.comp"));
		auto* buffer_binding = compute_program->GetBindingByName("buf");
		compute_bindings = std::make_unique<ShaderBindings>();
		compute_bindings->AddBufferBinding(buffer_binding->address.set, buffer_binding->address.binding, 0, compute_buffer->GetSize(), compute_buffer->GetBuffer()->Buffer());
	}

	void SceneRenderer::OnRecreateSwapchain(int32_t width, int32_t height)
	{
		render_graph->ClearCache();
		main_depth_attachment = std::make_unique<VulkanRenderTargetAttachment>(VulkanRenderTargetAttachment::Type::Depth, width, height, Format::D24_unorm_S8_uint);
	}

	void SceneRenderer::CreateDrawCalls()
	{
		auto* entity_manager = scene.GetEntityManager();
		auto list = entity_manager->GetChunkListsWithComponent<components::MeshRenderer>();
		create_draw_calls_system->ProcessChunks(list);
	}

	void SceneRenderer::UploadDrawCalls()
	{
		auto list = draw_call_manager->GetDrawCallChunks();
		upload_draw_calls_system->ProcessChunks(list);
	}

	static ShaderBufferStruct::Camera GetCameraData(ICameraParamsProvider* camera)
	{
		ShaderBufferStruct::Camera result;
		result.projectionMatrix = camera->cameraProjectionMatrix();
		result.projectionMatrix[1][1] *= -1;
		result.position = camera->cameraPosition();
		result.viewMatrix = camera->cameraViewMatrix();
		result.screenSize = camera->cameraViewSize();
		return result;
	}

	void SceneRenderer::RenderScene()
	{
		OPTICK_EVENT();

		auto* entity_manager = scene.GetEntityManager();

		CreateDrawCalls();
		draw_call_manager->ReleaseDrawCallLists();

		auto* context = Engine::GetVulkanContext();
		auto vk_device = context->GetDevice();
		auto vk_physical_device = context->GetPhysicalDevice();
		auto vk_swapchain = context->GetSwapchain();

		auto visible_objects = scene.visibleObjects(scene.GetCamera());

		// Shadow casters
		auto &visibleLights = scene.visibleLights(scene.GetCamera());
		shadow_casters.clear();
		for (auto &light : visibleLights) {
			if (light->castShadows()) {
				shadow_casters.push_back(
					std::make_pair(static_cast<IShadowCaster*>(light.get()), systems::CullingSystem(*draw_call_manager, *light))
				);
			}
		}

		auto &visibleProjectors = scene.visibleProjectors(scene.GetCamera());
		for (auto &projector : visibleProjectors) {
			if (projector->castShadows()) {
				shadow_casters.push_back(
					std::make_pair(static_cast<IShadowCaster*>(projector.get()), systems::CullingSystem(*draw_call_manager, *projector))
				);
			}
		}

		auto* object_params_buffer = scene_buffers->GetObjectParamsBuffer();
		auto* skinning_matrices_buffer = scene_buffers->GetSkinningMatricesBuffer();
		object_params_buffer->Map();
		skinning_matrices_buffer->Map();

		UploadDrawCalls();

		auto* camera_buffer = scene_buffers->GetCameraBuffer();
		camera_buffer->Map();
		camera_buffer->Append(GetCameraData(scene.GetCamera()));
		auto draw_calls = draw_call_manager->GetDrawCallChunks();

		systems::CullingSystem main_camera_culling_system(*draw_call_manager, *scene.GetCamera());
		main_camera_culling_system.ProcessChunks(draw_calls);

		for (auto& shadow_caster : shadow_casters)
		{
			shadow_caster.second.ProcessChunks(draw_calls); // Cull and fill draw call list
			auto offset = camera_buffer->Append(GetCameraData(shadow_caster.first));
			shadow_caster.first->cameraIndex(offset);
		}

		camera_buffer->Unmap();
		shadow_map->SetupShadowCasters(shadow_casters);

		// Light grid setup
		auto window_size = scene.GetCamera()->cameraViewSize();
		light_grid->Update(window_size.x, window_size.y);

		light_grid->appendLights(visibleLights, scene.GetCamera());
		light_grid->appendProjectors(visibleProjectors, scene.GetCamera());
		light_grid->upload();

		UpdateGlobalBindings();

		object_params_buffer->Unmap();
		skinning_matrices_buffer->Unmap();

		auto* swapchain = context->GetSwapchain();
		// Render Graph
		render_graph->Clear();
		
		struct PassInfo
		{
			graph::DependencyNode* color_output = nullptr;
			graph::DependencyNode* depth_output = nullptr;
			graph::DependencyNode* compute_output = nullptr;
		};

		auto* main_color = render_graph->RegisterAttachment(*swapchain->GetColorAttachment());
		auto* main_depth = render_graph->RegisterAttachment(*main_depth_attachment);
		auto* shadow_map = render_graph->RegisterAttachment(*shadowmap_atlas_attachment);
		auto* compute_buffer_resource = render_graph->RegisterBuffer(*compute_buffer->GetBuffer());

		/*auto compute_pass_info = render_graph->AddPass<PassInfo>("compute pass", [&](graph::IRenderPassBuilder& builder)
		{
			builder.SetCompute();

			PassInfo result;
			result.compute_output = builder.AddOutput(*compute_buffer_resource);
			return result;
		}, [&](VulkanRenderState& state)
		{
			state.RecordCompute(*compute_program, *compute_bindings, uvec3(128, 128, 1));
		});*/

		auto depth_pre_pass_info = render_graph->AddPass<PassInfo>("depth pre pass", [&](graph::IRenderPassBuilder& builder)
		{
			PassInfo result;
			result.depth_output = builder.AddOutput(*main_depth)->Clear(1.0f);
			return result;
		}, [&](VulkanRenderState& state)
		{
			RenderMode mode;
			mode.SetDepthWriteEnabled(true);
			mode.SetDepthTestEnabled(true);
			mode.SetDepthFunc(CompareOp::Less);

			state.SetRenderMode(mode);
			state.SetGlobalBindings(*global_shader_bindings);

			auto& render_queues = main_camera_culling_system.GetDrawCallList()->queues;
			for (auto* draw_call : render_queues[(size_t)RenderQueue::DepthOnly])
			{
				state.RenderDrawCall(draw_call, true);
			}
		});

		auto shadow_map_info = render_graph->AddPass<PassInfo>("shadow map", [&](graph::IRenderPassBuilder& builder)
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
			state.SetScissor(vec4(0, 0, ShadowAtlasSize(), ShadowAtlasSize()));
			ShaderBindings global_bindings = *global_shader_bindings;

			for (auto& shadow_caster : shadow_casters)
			{
				global_bindings.GetBufferBindings()[global_shader_binding_camera_index].offset = shadow_caster.first->cameraIndex();
				state.SetGlobalBindings(global_bindings);
				state.SetViewport(shadow_caster.first->cameraViewport());
				auto& render_queues = shadow_caster.second.GetDrawCallList()->queues;
				for (auto* draw_call : render_queues[(size_t)RenderQueue::DepthOnly])
				{
					state.RenderDrawCall(draw_call, true);
				}
			}

		});

		auto* debug_draw = Engine::Get()->GetDebugDraw();
		auto& debug_draw_calls_lines = debug_draw->GetLineDrawCalls();
		auto& debug_draw_calls_points = debug_draw->GetPointDrawCalls();


		auto main_pass_info = render_graph->AddPass<PassInfo>("main", [&](graph::IRenderPassBuilder& builder)
		{
			PassInfo result;
			builder.AddInput(*depth_pre_pass_info.depth_output, graph::InputUsage::DepthAttachment);
			builder.AddInput(*shadow_map_info.depth_output);
			result.color_output = builder.AddOutput(*main_color)->Clear(vec4(0));
			return result;
		}, [&](VulkanRenderState& state)
		{
			RenderMode mode;
			mode.SetDepthWriteEnabled(false);
			mode.SetDepthTestEnabled(true);
			mode.SetDepthFunc(CompareOp::LessOrEqual);

			state.SetRenderMode(mode);
			state.SetGlobalBindings(*global_shader_bindings);

			auto& render_queues = main_camera_culling_system.GetDrawCallList()->queues;
			for (auto* draw_call : render_queues[(size_t)RenderQueue::Opaque])
			{
				state.RenderDrawCall(draw_call, false);
			}

			// Debug
			mode.SetDepthWriteEnabled(false);
			mode.SetDepthTestEnabled(false);
			mode.SetPolygonMode(PolygonMode::Line);
			mode.SetPrimitiveTopology(PrimitiveTopology::LineList);

			state.SetRenderMode(mode);

			for (auto& draw_call : debug_draw_calls_lines)
			{
				state.RenderDrawCall(&draw_call, false);
			}

			mode.SetPolygonMode(PolygonMode::Point);
			mode.SetPrimitiveTopology(PrimitiveTopology::PointList);

			state.SetRenderMode(mode);

			for (auto& draw_call : debug_draw_calls_points)
			{
				state.RenderDrawCall(&draw_call, false);
			}

			DebugUI::Render(state);
		});

		render_graph->Prepare();
		render_graph->Render();
	}

	std::tuple<vk::Buffer, size_t> SceneRenderer::GetBufferData(ShaderBufferName buffer_name)
	{
		vk::Buffer buffer = nullptr;
		size_t size;
		switch (buffer_name)
		{
		case ShaderBufferName::ObjectParams:
		{
			auto* object_params_buffer = scene_buffers->GetObjectParamsBuffer();
			buffer = object_params_buffer->GetBuffer()->Buffer();
			size = object_params_buffer->GetElementSize();
			break;
		}

		case ShaderBufferName::Camera:
		{
			auto* camera_buffer = scene_buffers->GetCameraBuffer();
			buffer = camera_buffer->GetBuffer()->Buffer();
			size = camera_buffer->GetElementSize();
			break;
		}

		case ShaderBufferName::SkinningMatrices:
		{
			auto* skinning_matrices_buffer = scene_buffers->GetSkinningMatricesBuffer();
			buffer = skinning_matrices_buffer->GetBuffer()->Buffer();
			size = skinning_matrices_buffer->GetElementSize();
			break;
		}

		case ShaderBufferName::Projector:
		{
			auto* uniform_buffer = light_grid->GetProjectorBuffer();
			buffer = uniform_buffer->GetBuffer()->Buffer();
			size = uniform_buffer->GetSize();
			break;
		}

		case ShaderBufferName::Light:
		{
			auto* uniform_buffer = light_grid->GetLightsBuffer();
			buffer = uniform_buffer->GetBuffer()->Buffer();
			size = uniform_buffer->GetSize();
			break;
		}

		case ShaderBufferName::LightIndices:
		{
			auto* uniform_buffer = light_grid->GetLightIndexBuffer();
			buffer = uniform_buffer->GetBuffer()->Buffer();
			size = uniform_buffer->GetSize();
			break;
		}

		case ShaderBufferName::LightGrid:
		{
			auto* uniform_buffer = light_grid->GetLightGridBuffer();
			buffer = uniform_buffer->GetBuffer()->Buffer();
			size = uniform_buffer->GetSize();
			break;
		}

		default:
			throw std::runtime_error("unknown shader buffer");
		}

		assert(buffer && "buffer should be defined");

		return std::make_tuple(buffer, size);
	}

	Texture* SceneRenderer::GetTexture(ShaderTextureName texture_name, const Material& material)
	{
		switch (texture_name)
		{
		case ShaderTextureName::Texture0:
			return material.texture0().get();

		case ShaderTextureName::ShadowMap:
			return shadowmap_atlas_attachment->GetTexture(0).get();

		default:
			throw std::runtime_error("unknown texture");
		}

		return nullptr;
	}

	void SceneRenderer::SetupShaderBindings(const Material& material, const ShaderProgram::DescriptorSet& descriptor_set, ShaderBindings& bindings)
	{
		for (auto& binding : descriptor_set.bindings)
		{
			auto& address = binding.address;
			switch (binding.type)
			{
			case ShaderProgram::BindingType::Sampler:
				bindings.AddTextureBinding(address.set, address.binding, GetTexture((ShaderTextureName)binding.id, material));
				break;

			case ShaderProgram::BindingType::UniformBuffer:
			case ShaderProgram::BindingType::StorageBuffer:
			{
				auto buffer_data = GetBufferData((ShaderBufferName)binding.id);
				vk::Buffer buffer;
				size_t size;
				std::tie(buffer, size) = buffer_data;

				bindings.AddBufferBinding(address.set, address.binding, 0, size, buffer);
				break;
			}

			default:
				throw std::runtime_error("unknown shader binding");
			}
		}
	}

	void SceneRenderer::UpdateGlobalBindings()
	{
		global_shader_bindings = std::make_unique<ShaderBindings>();
		Material material;
		material.lightingEnabled(true); // For now it's enough to get all the global bindings
		auto vertex_name_hash = material.GetVertexShaderNameHash();
		auto vertex_hash =  ShaderCache::GetCombinedHash(vertex_name_hash, material.shaderCaps());
		auto fragment_hash = ShaderCache::GetCombinedHash(material.GetFragmentShaderNameHash(), material.shaderCaps());

		auto* shader = shader_cache->GetShaderProgram(vertex_hash, fragment_hash);
		auto* descriptor_set = shader->GetDescriptorSet(DescriptorSet::Global);

		SetupShaderBindings(material, *descriptor_set, *global_shader_bindings);

		// Getting camera binding index since it will be modified within the shadowmap pass
		global_shader_binding_camera_index = -1;
		for (int i = 0; i < global_shader_bindings->GetBufferBindings().size(); i++)
			if (global_shader_bindings->GetBufferBindings()[i].buffer == vk::Buffer(scene_buffers->GetCameraBuffer()->GetBuffer()->Buffer()))
			{
				global_shader_binding_camera_index = i;
				break;
			}

		assert(global_shader_binding_camera_index != -1);
	}

} }
