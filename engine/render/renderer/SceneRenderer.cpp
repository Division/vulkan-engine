#include "SceneRenderer.h"
#include "scene/Scene.h"
#include "render/device/VulkanUtils.h"
#include "render/shader/Shader.h"
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
#include "render/mesh/Mesh.h"
#include "render/shader/Shader.h"
#include "render/shader/ShaderCache.h"
#include "render/shader/ShaderBindings.h"
#include "render/renderer/RenderOperation.h"
#include "render/renderer/RenderGraph.h"
#include "render/material/Material.h"
#include "render/buffer/UniformBuffer.h"
#include "objects/LightObject.h"
#include "objects/Projector.h"
#include "SceneBuffers.h"
#include "objects/Camera.h"

using namespace core;
using namespace core::Device;


namespace core { namespace render {

	SceneRenderer::~SceneRenderer() = default;

	SceneRenderer::SceneRenderer(ShaderCache* shader_cache)
		: shader_cache(shader_cache)
	{
		scene_buffers = std::make_unique<SceneBuffers>();
		light_grid = std::make_unique<LightGrid>();
		render_graph = std::make_unique<graph::RenderGraph>();

		auto* context = Engine::GetVulkanContext();
		auto vk_swapchain = context->GetSwapchain();

		depth_only_fragment_shader_hash = ShaderCache::GetShaderPathHash(L"shaders/noop.frag");

		temp_color_attachment = std::make_unique<VulkanRenderTargetAttachment>(VulkanRenderTargetAttachment::Type::Color, 800, 600, Format::R8G8B8A8_norm);

		/*VulkanRenderTargetInitializer rt_initializer(1, 1);
		color_target = std::make_unique<VulkanRenderTarget>(rt_initializer);
		temp_target1 = std::make_unique<VulkanRenderTarget>(rt_initializer);
		temp_target2 = std::make_unique<VulkanRenderTarget>(rt_initializer);*/
	}

	ShaderBufferStruct::Camera camera_data;
	ShaderBufferStruct::ObjectParams object_data[4];

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

	void SceneRenderer::AddRenderOperation(core::Device::RenderOperation& rop, RenderQueue queue)
	{
		auto* draw_call = GetDrawCall(rop);
		render_queues[(size_t)queue].push_back(draw_call);

		if (queue == RenderQueue::Opaque)
		{
			auto* depth_draw_call = GetDrawCall(rop, true);
			render_queues[(size_t)RenderQueue::DepthOnly].push_back(depth_draw_call);
		}
	}

	void SceneRenderer::RenderScene(Scene* scene)
	{
		auto* context = Engine::GetVulkanContext();
		auto vk_device = context->GetDevice();
		auto vk_physical_device = context->GetPhysicalDevice();
		auto vk_swapchain = context->GetSwapchain();

		for (auto& queue : render_queues)
			queue.clear();

		rop_transform_cache.clear();
		auto visible_objects = scene->visibleObjects(scene->GetCamera());

		auto* camera_buffer = scene_buffers->GetCameraBuffer();
		camera_buffer->Map();
		camera_buffer->Append(GetCameraData(scene->GetCamera()));
		camera_buffer->Unmap();

		// Shadow casters
		auto &visibleLights = scene->visibleLights(scene->GetCamera());
		//_shadowCasters.clear();
		for (auto &light : visibleLights) {
			if (light->castShadows()) {
				//_shadowCasters.push_back(std::static_pointer_cast<IShadowCaster>(light));
			}
		}

		auto &visibleProjectors = scene->visibleProjectors(scene->GetCamera());
		for (auto &projector : visibleProjectors) {
			if (projector->castShadows()) {
				//_shadowCasters.push_back(std::static_pointer_cast<IShadowCaster>(projector));
			}
		}

		// Light grid setup
		auto window_size = scene->GetCamera()->cameraViewSize();
		light_grid->Update(window_size.x, window_size.y);
		//_shadowMap->setupRenderPasses(_shadowCasters); // Should go BEFORE lightgrid setup

		auto lights = scene->visibleLights(scene->GetCamera());
		light_grid->appendLights(lights, scene->GetCamera());
		auto projectors = scene->visibleProjectors(scene->GetCamera());
		light_grid->appendProjectors(projectors, scene->GetCamera());
		light_grid->upload();

		auto* object_params_buffer = scene_buffers->GetObjectParamsBuffer();
		auto* skinning_matrices_buffer = scene_buffers->GetSkinningMatricesBuffer();
		object_params_buffer->Map();
		skinning_matrices_buffer->Map();
		for (auto& object : visible_objects)
		{
			object->render(*this);
		}
		object_params_buffer->Unmap();
		skinning_matrices_buffer->Unmap();

		auto* swapchain = context->GetSwapchain();
		// Render Graph
		render_graph->Clear();
		
		struct PassInfo
		{
			graph::DependencyNode* color_output = nullptr;
			graph::DependencyNode* depth_output = nullptr;
		};

		auto* main_color = render_graph->RegisterAttachment(*swapchain->GetColorAttachment());
		auto* main_depth = render_graph->RegisterAttachment(*swapchain->GetDepthAttachment());

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
			for (auto* draw_call : render_queues[(size_t)RenderQueue::DepthOnly])
			{
				state.RenderDrawCall(draw_call);
			}
		});

		auto main_pass_info = render_graph->AddPass<PassInfo>("main", [&](graph::IRenderPassBuilder& builder)
		{
			PassInfo result;
			builder.AddInput(*depth_pre_pass_info.depth_output, graph::InputUsage::DepthAttachment);
			result.color_output = builder.AddOutput(*main_color)->Clear(vec4(0));
			return result;
		}, [&](VulkanRenderState& state)
		{
			RenderMode mode;
			mode.SetDepthWriteEnabled(false);
			mode.SetDepthTestEnabled(true);
			mode.SetDepthFunc(CompareOp::LessOrEqual);

			state.SetRenderMode(mode);
			for (auto* draw_call : render_queues[(size_t)RenderQueue::Opaque])
			{
				state.RenderDrawCall(draw_call);
			}
		});

		render_graph->Prepare();
		render_graph->Render();

		ReleaseDrawCalls();
	}
	
	Texture* GetTextureFromROP(RenderOperation& rop, ShaderTextureName texture_name)
	{
		auto* material = rop.material.get();
		switch (texture_name)
		{
		case ShaderTextureName::Texture0:
			return material->texture0().get();
		}

		return nullptr;
	}

	std::tuple<vk::Buffer, size_t, size_t> SceneRenderer::GetBufferFromROP(RenderOperation& rop, ShaderBufferName buffer_name)
	{
		vk::Buffer buffer = nullptr;
		size_t offset;
		size_t size;
		switch (buffer_name)
		{
		case ShaderBufferName::ObjectParams:
			{
				auto* object_params_buffer = scene_buffers->GetObjectParamsBuffer();
				buffer = object_params_buffer->GetBuffer()->Buffer();
				offset = object_params_buffer->Append(*rop.object_params);
				size = object_params_buffer->GetElementSize();
				break;
			}

		case ShaderBufferName::Camera:
		{
			auto* camera_buffer = scene_buffers->GetCameraBuffer();
			buffer = camera_buffer->GetBuffer()->Buffer();
			offset = 0;
			size = camera_buffer->GetElementSize();
			break;
		}

		case ShaderBufferName::SkinningMatrices:
		{
			auto* skinning_matrices_buffer = scene_buffers->GetSkinningMatricesBuffer();
			buffer = skinning_matrices_buffer->GetBuffer()->Buffer();
			offset = skinning_matrices_buffer->Append(*rop.skinning_matrices);
			size = skinning_matrices_buffer->GetElementSize();
			break;
		}

		case ShaderBufferName::Projector:
		{
			auto* uniform_buffer = light_grid->GetProjectorBuffer();
			buffer = uniform_buffer->GetBuffer()->Buffer();
			offset = 0;
			size = uniform_buffer->GetSize();
			break;
		}

		case ShaderBufferName::Light:
		{
			auto* uniform_buffer = light_grid->GetLightsBuffer();
			buffer = uniform_buffer->GetBuffer()->Buffer();
			offset = 0;
			size = uniform_buffer->GetSize();
			break;
		}

		case ShaderBufferName::LightIndices:
		{
			auto* uniform_buffer = light_grid->GetLightIndexBuffer();
			buffer = uniform_buffer->GetBuffer()->Buffer();
			offset = 0;
			size = uniform_buffer->GetSize();
			break;
		}

		case ShaderBufferName::LightGrid:
		{
			auto* uniform_buffer = light_grid->GetLightGridBuffer();
			buffer = uniform_buffer->GetBuffer()->Buffer();
			offset = 0;
			size = uniform_buffer->GetSize();
			break;
		}

		default:
			throw std::runtime_error("unknown shader buffer");
		}

		assert(buffer && "buffer should be defined");

		return std::make_tuple(buffer, offset, size);
	}

	void SceneRenderer::SetupShaderBindings(RenderOperation& rop, ShaderProgram& shader, ShaderBindings& bindings)
	{
		for (auto& set : shader.GetDescriptorSets())
		{
			for (auto& binding : set.bindings)
			{
				auto& address = binding.address;
				switch (binding.type)
				{
					case ShaderProgram::BindingType::Sampler:
						bindings.AddTextureBinding(address.set, address.binding, GetTextureFromROP(rop, (ShaderTextureName)binding.id));
						break;

					case ShaderProgram::BindingType::UniformBuffer:
					case ShaderProgram::BindingType::StorageBuffer:
					{
						auto buffer_data = GetBufferFromROP(rop, (ShaderBufferName)binding.id);
						vk::Buffer buffer;
						size_t offset;
						size_t size;
						std::tie(buffer, offset, size) = buffer_data;

						bindings.AddBufferBinding(address.set, address.binding, offset, size, buffer);
						break;
					}

					default:
						throw std::runtime_error("unknown shader binding");
				}
			}
		}
	}

	DrawCall* SceneRenderer::GetDrawCall(RenderOperation& rop, bool depth_only)
	{
		auto draw_call = draw_call_pool.Obtain();
		draw_call->mesh = nullptr;
		draw_call->shader = nullptr;
		draw_call->shader_bindings->Clear();
		assert(rop.object_params && "Object params data must be set");

		draw_call->mesh = rop.mesh.get();

		auto caps = depth_only ? ShaderCapsSet() : rop.material->shaderCaps();
		if (rop.skinning_matrices)
			caps.addCap(ShaderCaps::Skinning);

		auto vertex_name_hash = depth_only ? rop.material->GetVertexShaderDepthOnlyNameHash() : rop.material->GetVertexShaderNameHash();
		auto vertex_hash =  ShaderCache::GetCombinedHash(vertex_name_hash, caps);
		auto fragment_hash = depth_only ? depth_only_fragment_shader_hash : ShaderCache::GetCombinedHash(rop.material->GetFragmentShaderNameHash(), caps);

		draw_call->shader = shader_cache->GetShaderProgram(vertex_hash, fragment_hash);
		SetupShaderBindings(rop, *draw_call->shader, *draw_call->shader_bindings);

		auto* result = draw_call.get();
		used_draw_calls.push_back(std::move(draw_call));
		return result;
	}

	void SceneRenderer::ReleaseDrawCalls()
	{
		for (auto& draw_call : used_draw_calls)
		{
			draw_call_pool.Release(std::move(draw_call));
		}

		used_draw_calls.clear();
	}

} }
