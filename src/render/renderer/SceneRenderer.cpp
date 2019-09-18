#include "SceneRenderer.h"
#include "scene/Scene.h"
#include "render/device/VulkanUtils.h"
#include "render/shader/Shader.h"
#include "../game/Game.h"
#include "CommonIncludes.h"
#include "../game/GameUtils.h"
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
#include "resources/ModelBundle.h"
#include "loader/ModelLoader.h"
#include "render/mesh/Mesh.h"
#include "render/shader/Shader.h"

using namespace core;
using namespace core::Device;

VkImage textureImage;
VkDeviceMemory textureImageMemory;
VkImageView textureImageView;
VkSampler textureSampler;

std::unique_ptr<VulkanBuffer> vertexBuffer;
//VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferMemory;
VkBuffer indexBuffer;
VkDeviceMemory indexBufferMemory;

std::vector<VkBuffer> uniformBuffers;
std::vector<VkDeviceMemory> uniformBuffersMemory;

VkDescriptorPool descriptorPool;
std::vector<VkDescriptorSet> descriptorSets;

std::vector<VkCommandBuffer> commandBuffers;
std::shared_ptr<ModelBundle> bundle;
std::shared_ptr<const Mesh> test_mesh;

ShaderProgram program;
std::unique_ptr<VulkanPipeline> pipeline;

void CreateCommandBuffers(const uint32_t in_flight_count) {
	auto* engine = Engine::Get();
	auto* device = engine->GetDevice();
	auto* context = device->GetContext();
	auto commandPool = context->GetCommandPool();
	auto render_pass = context->GetRenderPass();

	auto vertex_buffer = test_mesh->vertexBuffer();
	auto index_buffer = test_mesh->indexBuffer();

	commandBuffers.resize(in_flight_count);

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();
	auto vk_device = context->GetDevice();

	if (vkAllocateCommandBuffers(vk_device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}

	for (size_t i = 0; i < commandBuffers.size(); i++) {
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = render_pass->GetRenderPass();
		renderPassInfo.framebuffer = context->GetFramebuffer(i);
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = context->GetExtent();

		VkClearValue clearColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		//vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, context->GetPipeline());
		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetPipeline());

		VkBuffer vertexBuffers[] = { vertex_buffer->Buffer() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(commandBuffers[i], index_buffer->Buffer(), 0, VK_INDEX_TYPE_UINT16);

		//vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, context->GetPipelineLayout(), 0, 1, &descriptorSets[i], 0, nullptr);
		vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetPipelineLayout(), 0, 1, &descriptorSets[i], 0, nullptr);

		vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(test_mesh->indexCount()), 1, 0, 0, 0);

		vkCmdEndRenderPass(commandBuffers[i]);

		if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}
}

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

	Device::ShaderProgram program;

	SceneRenderer::SceneRenderer()
	{
		auto* engine = Engine::Get();
		auto* device = engine->GetDevice();
		auto* context = device->GetContext();
		auto vk_device = context->GetDevice();

		bundle = loader::loadModel("resources/level/props.mdl");
		test_mesh = bundle->getMesh("Drawer_Preset_02_meshShape");

		auto vertex_data = VulkanUtils::ReadFile("shaders/vert.spv");
		auto fragment_data = VulkanUtils::ReadFile("shaders/frag.spv");
		ShaderModule vertex(vertex_data.data(), vertex_data.size());
		ShaderModule fragment(fragment_data.data(), fragment_data.size());

		program.AddModule(std::move(vertex), ShaderProgram::Stage::Vertex);
		program.AddModule(std::move(fragment), ShaderProgram::Stage::Fragment);
		program.Prepare();

		const uint32_t in_flight_count = context->GetSwapchainImageCount(); // check is there a reason for it to be separate
		CreateTextureImage(textureImage, textureImageMemory);
		textureImageView = CreateTextureImageView(vk_device, textureImage);
		CreateTextureSampler(vk_device, textureSampler);
		CreateUniformBuffers(in_flight_count, uniformBuffers, uniformBuffersMemory);
		CreateDescriptorPool(in_flight_count, descriptorPool);

		CreateDescriptorSets(in_flight_count, textureImageView, textureSampler,
			uniformBuffers, descriptorPool, program.GetDescriptorSetLayout(), descriptorSets);

		VulkanPipelineInitializer pipeline_initializer(&program, context->GetRenderPass(), test_mesh.get());
		pipeline = std::make_unique<VulkanPipeline>(pipeline_initializer);

		CreateCommandBuffers(in_flight_count);
	}

	void SceneRenderer::RenderScene(Scene* scene)
	{
		auto* engine = Engine::Get();
		auto* device = engine->GetDevice();
		auto* context = device->GetContext();
		auto vk_device = context->GetDevice();
		auto vk_physical_device = context->GetPhysicalDevice();
		auto vk_swapchain = context->GetSwapchain();

		UpdateUniformBuffer(context->GetCurrentFrame());
		context->AddFrameCommandBuffer(commandBuffers[context->GetCurrentFrame()]);
	}

} }
