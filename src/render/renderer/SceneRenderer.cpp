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

using namespace core;
using namespace core::Device;

std::vector<VkBuffer> uniformBuffers;
std::vector<VkDeviceMemory> uniformBuffersMemory;

std::shared_ptr<core::Device::Texture> texture;

namespace core {
	namespace render {
		std::vector<vk::DescriptorSet> descriptorSets;
	}
}

std::shared_ptr<ModelBundle> bundle;
std::shared_ptr<const Mesh> test_mesh;

ShaderProgram program;

struct UniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

void UpdateUniformBuffer(uint32_t currentImage) {
	auto* engine = Engine::Get();
	auto* device = engine->GetDevice();
	auto* context = device->GetContext();
	auto vk_device = context->GetDevice();
	auto vk_physical_device = context->GetPhysicalDevice();

	float time = engine->time();
	auto swapChainExtent = context->GetExtent();

	UniformBufferObject ubo = {};
	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
	ubo.proj[1][1] *= -1;

	void* data;
	vkMapMemory(vk_device, uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(vk_device, uniformBuffersMemory[currentImage]);
}

namespace core { namespace render {

	ShaderProgram program;

	static void CreateUniformBuffers(uint32_t in_flight_count, std::vector<VkBuffer>& uniformBuffers, std::vector<VkDeviceMemory>& uniformBuffersMemory) {
		auto* engine = Engine::Get();
		auto* device = engine->GetDevice();
		auto* context = device->GetContext();
		auto vk_device = context->GetDevice();
		auto vk_physical_device = context->GetPhysicalDevice();

		VkDeviceSize bufferSize = sizeof(UniformBufferObject);

		uniformBuffers.resize(in_flight_count);
		uniformBuffersMemory.resize(in_flight_count);

		for (size_t i = 0; i < in_flight_count; i++) {
			VulkanUtils::CreateBuffer(vk_device, vk_physical_device, bufferSize,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				uniformBuffers[i], uniformBuffersMemory[i]);
		}
	}

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

		const uint32_t in_flight_count = context->GetSwapchainImageCount();
		CreateUniformBuffers(in_flight_count, uniformBuffers, uniformBuffersMemory);
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

		UpdateUniformBuffer(context->GetCurrentFrame());
		
		RenderOperation rop;
		auto material = std::make_shared<Material>();
		material->texture0(texture);
		rop.material = material;
		rop.mesh = test_mesh;

		auto* command_buffer = render_state->BeginRendering(*context->GetMainRenderTarget());
		auto* draw_call = GetDrawCall(rop);
		render_state->RenderDrawCall(draw_call);
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

	vk::Buffer GetBufferFromROP(RenderOperation& rop, ShaderBufferName buffer_name)
	{
		vk::Buffer result = nullptr;
		switch (buffer_name)
		{
		case ShaderBufferName::ObjectParams:
			result = uniformBuffers[Engine::GetVulkanContext()->GetCurrentFrame()];
			break;

		default:
			throw std::runtime_error("unknown shader buffer");
		}

		assert(result && "buffer should bne defined");

		return result;
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
					bindings.AddBufferBinding(address.set, address.binding, sizeof(ShaderBufferName), GetBufferFromROP(rop, (ShaderBufferName)binding.id));
					break;
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
