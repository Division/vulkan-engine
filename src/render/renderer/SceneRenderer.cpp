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

using namespace core;
using namespace core::Device;

std::shared_ptr<core::Device::Texture> texture;
std::shared_ptr<ModelBundle> bundle;
std::shared_ptr<const Mesh> test_mesh;


struct UniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

ShaderProgram program;
std::unique_ptr<UniformBuffer<UniformBufferObject>> uniform_buffer;

void UpdateUniformBuffer(RenderOperation& rop, float index) {
	auto* engine = Engine::Get();
	auto* context = Engine::GetVulkanContext();
	float time = engine->time();
	auto swapChainExtent = context->GetExtent();
	UniformBufferObject ubo = {};
	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f) + index, glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
	ubo.proj[1][1] *= -1;

	rop.object_params_buffer = uniform_buffer->GetBuffer()->Buffer();
	rop.object_params_buffer_offset = uniform_buffer->Append(ubo);
}

namespace core { namespace render {

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

		uniform_buffer = std::make_unique<UniformBuffer<UniformBufferObject>>(1024);
	}

	void SceneRenderer::RenderScene(Scene* scene)
	{
		auto* engine = Engine::Get();
		auto* device = engine->GetDevice();
		auto* context = device->GetContext();
		auto vk_device = context->GetDevice();
		auto vk_physical_device = context->GetPhysicalDevice();
		auto vk_swapchain = context->GetSwapchain();

		auto* render_state = context->GetRenderState();

		
		RenderOperation rop;
		auto material = std::make_shared<Material>();
		material->texture0(texture);
		rop.material = material;
		rop.mesh = test_mesh;

		auto* command_buffer = render_state->BeginRendering(*context->GetMainRenderTarget());

		uniform_buffer->Map();
		for (int i = 0; i < 4; i++)
		{
			UpdateUniformBuffer(rop, i * M_PI / 2);
			auto* draw_call = GetDrawCall(rop);
			render_state->RenderDrawCall(draw_call);
		}
		uniform_buffer->Unmap();

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

	std::tuple<vk::Buffer, size_t, size_t> GetBufferFromROP(RenderOperation& rop, ShaderBufferName buffer_name)
	{
		vk::Buffer buffer = nullptr;
		size_t offset;
		size_t size;
		switch (buffer_name)
		{
		case ShaderBufferName::ObjectParams:
			buffer = rop.object_params_buffer;
			offset = rop.object_params_buffer_offset;
			size = sizeof(UniformBufferObject); // todo: fix
			break;

		default:
			throw std::runtime_error("unknown shader buffer");
		}

		assert(buffer && "buffer should be defined");

		return std::make_tuple(buffer, offset, size);
	}

	void SetupShaderBindings(RenderOperation& rop, ShaderProgram& shader, ShaderBindings& bindings)
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
