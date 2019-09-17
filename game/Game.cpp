#include "Game.h"
#include "CommonIncludes.h"
#include "GameUtils.h"
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

const std::vector<Vertex> vertices = {
	{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
	{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
	{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
};

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

void Game::init()
{
	auto* engine = Engine::Get();
	auto* device = engine->GetDevice();
	auto* context = device->GetContext();
	auto vk_device = context->GetDevice();
	auto vk_physical_device = context->GetPhysicalDevice();

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
	vertexBuffer = std::move(CreateVertexBuffer(vertices));
	//CreateVertexBuffer(vertices, vertexBuffer, vertexBufferMemory);
	//CreateIndexBuffer(indices, indexBuffer, indexBufferMemory);
	CreateUniformBuffers(in_flight_count, uniformBuffers, uniformBuffersMemory);
	CreateDescriptorPool(in_flight_count, descriptorPool);

	CreateDescriptorSets(in_flight_count, textureImageView, textureSampler,
		uniformBuffers, descriptorPool, program.GetDescriptorSetLayout(), descriptorSets);

	VulkanPipelineInitializer pipeline_initializer(&program, context->GetRenderPass(), test_mesh.get());
	pipeline = std::make_unique<VulkanPipeline>(pipeline_initializer);

	CreateCommandBuffers(in_flight_count);
}

void Game::update(float dt)
{
	auto* engine = Engine::Get();
	auto* device = engine->GetDevice();
	auto* context = device->GetContext();
	auto vk_device = context->GetDevice();
	auto vk_physical_device = context->GetPhysicalDevice();
	auto vk_swapchain = context->GetSwapchain();
	VkFence current_fence = context->GetInFlightFence();
	VkSemaphore image_available_semaphone = context->GetImageAvailableSemaphore();
	VkSemaphore render_finished_semaphore = context->GetRenderFinishedSemaphore();

	vkWaitForFences(vk_device, 1, &current_fence, VK_TRUE, std::numeric_limits<uint64_t>::max());
	context->GetUploader()->ProcessUpload();

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(vk_device, vk_swapchain->GetSwapchain(), std::numeric_limits<uint64_t>::max(), image_available_semaphone, VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		throw new std::runtime_error("recreate not supported");
		//recreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	UpdateUniformBuffer(imageIndex);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { image_available_semaphone };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

	VkSemaphore signalSemaphores[] = { render_finished_semaphore };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(vk_device, 1, &current_fence);

	// Queue waits for the image_available_semaphone which is signaled after vkAcquireNextImageKHR succeeds
	if (vkQueueSubmit(context->GetGraphicsQueue(), 1, &submitInfo, current_fence) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}


	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { context->GetSwapchain()->GetSwapchain() };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR(context->GetPresentQueue(), &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR /*|| framebufferResized*/) {
		throw new std::runtime_error("recreate not supported");
		//framebufferResized = false;
		//recreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}
}

void Game::cleanup()
{
	vertexBuffer = nullptr;
	bundle = nullptr;
}