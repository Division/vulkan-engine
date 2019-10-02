#include "SceneRenderer.h"
#include "scene/Scene.h"
#include "render/device/VulkanUtils.h"
#include "render/shader/Shader.h"
#include "../game/Game.h"
#include "CommonIncludes.h"
#include "render/device/Device.h"
#include "Engine.h"
#include "render/device/Device.h"
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
#include "render/texture/Texture.h"
#include "render/mesh/Mesh.h"
#include "render/shader/Shader.h"
#include "render/shader/ShaderBindings.h"
#include "render/renderer/RenderOperation.h"
#include "render/material/Material.h"
#include "render/buffer/UniformBuffer.h"
#include "SceneBuffers.h"
#include "objects/Camera.h"

using namespace core;
using namespace core::Device;

std::shared_ptr<core::Device::Texture> texture;
std::shared_ptr<ModelBundle> bundle;
std::shared_ptr<const Mesh> test_mesh;
ShaderProgram program;

namespace core { namespace render {

	SceneRenderer::~SceneRenderer() = default;

	SceneRenderer::SceneRenderer()
	{
		auto* engine = Engine::Get();
		auto* device = engine->GetDevice();
		auto* context = device->GetContext();
		auto vk_device = context->GetDevice();

		bundle = loader::loadModel("resources/level/props.mdl");
		test_mesh = bundle->getMesh("Drawer_Preset_02_meshShape");

		texture = loader::LoadTexture("resources/level/Atlas_Props_02.jpg");
		
		auto vertex_data = VulkanUtils::ReadFile("shaders/vert.spv");
		auto fragment_data = VulkanUtils::ReadFile("shaders/frag.spv");
		ShaderModule vertex(vertex_data.data(), vertex_data.size());
		ShaderModule fragment(fragment_data.data(), fragment_data.size());

		program.AddModule(std::move(vertex), ShaderProgram::Stage::Vertex);
		program.AddModule(std::move(fragment), ShaderProgram::Stage::Fragment);
		program.Prepare();

		scene_buffers = std::make_unique<SceneBuffers>();
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
		result.screenSize = camera->cameraViewport();
		return result;
	}

	void SceneRenderer::AddRenderOperation(core::Device::RenderOperation& rop, RenderQueue queue)
	{
		auto* draw_call = GetDrawCall(rop);
		render_queues[(size_t)queue].push_back(draw_call);
	}

	void SceneRenderer::RenderScene(Scene* scene)
	{
		auto* engine = Engine::Get();
		auto* device = engine->GetDevice();
		auto* context = device->GetContext();
		auto vk_device = context->GetDevice();
		auto vk_physical_device = context->GetPhysicalDevice();
		auto vk_swapchain = context->GetSwapchain();

		for (auto& queue : render_queues)
			queue.clear();

		rop_transform_cache.clear();
		auto visible_objects = scene->visibleObjects(scene->GetCamera());
		if (!visible_objects.size())
			return;

		auto* camera_buffer = scene_buffers->GetCameraBuffer();
		camera_buffer->Map();
		camera_buffer->Append(GetCameraData(scene->GetCamera()));
		camera_buffer->Unmap();

		auto* object_params_buffer = scene_buffers->GetObjectParamsBuffer();
		object_params_buffer->Map();
		for (auto& object : visible_objects)
		{
			object->render(*this);
		}
		object_params_buffer->Unmap();

		auto* render_state = context->GetRenderState();
		auto* command_buffer = render_state->BeginRendering(*context->GetMainRenderTarget());
		for (auto* draw_call : render_queues[(size_t)RenderQueue::Opaque])
		{
			render_state->RenderDrawCall(draw_call);
		}
		render_state->EndRendering();

		context->AddFrameCommandBuffer(command_buffer->GetCommandBuffer());
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
					{
						auto buffer_data = GetBufferFromROP(rop, (ShaderBufferName)binding.id);
						vk::Buffer buffer;
						size_t offset;
						size_t size;
						std::tie(buffer, offset, size) = buffer_data;

						bindings.AddBufferBinding(address.set, address.binding, offset, size, buffer);
						break;
					}
				}
			}
		}
	}

	DrawCall* SceneRenderer::GetDrawCall(RenderOperation& rop)
	{
		auto draw_call = draw_call_pool.Obtain();
		draw_call->mesh = nullptr;
		draw_call->shader = nullptr;
		draw_call->shader_bindings->Clear();
		assert(rop.object_params && "Object params data must be set");

		if (rop.shader)
			draw_call->shader = rop.shader;
		else
		{
			throw std::runtime_error("shader from material not supported");
		}

		draw_call->mesh = rop.mesh.get();
		draw_call->shader = &program; // TODO: use material
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
