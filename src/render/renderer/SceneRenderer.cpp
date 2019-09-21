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
#include "render/device/VulkanRenderState.h"
#include "render/device/VulkanRenderTarget.h"
#include "render/device/VkObjects.h"
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

namespace core {
	namespace render {
		std::vector<vk::DescriptorSet> descriptorSets;
	}
}

std::shared_ptr<ModelBundle> bundle;
std::shared_ptr<const Mesh> test_mesh;

ShaderProgram program;
std::unique_ptr<VulkanPipeline> pipeline;

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

		const uint32_t in_flight_count = context->GetSwapchainImageCount();
		CreateTextureImage(textureImage, textureImageMemory);
		textureImageView = CreateTextureImageView(vk_device, textureImage);
		CreateTextureSampler(vk_device, textureSampler);
		CreateUniformBuffers(in_flight_count, uniformBuffers, uniformBuffersMemory);
		CreateDescriptorPool(in_flight_count, descriptorPool);

		CreateDescriptorSets(in_flight_count, textureImageView, textureSampler,
			uniformBuffers, descriptorPool, program.GetDescriptorSetLayout(), descriptorSets);

		::RenderMode render_mode;
		VulkanPipelineInitializer pipeline_initializer(&program, context->GetRenderPass(), test_mesh.get(), &render_mode);
		pipeline = std::make_unique<VulkanPipeline>(pipeline_initializer);
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

		auto* command_buffer = render_state->BeginRendering(*context->GetMainRenderTarget());
		render_state->SetShader(program);
		render_state->RenderMesh(*test_mesh.get());
		render_state->EndRendering();

		context->AddFrameCommandBuffer(command_buffer->GetCommandBuffer());
	}

} }
