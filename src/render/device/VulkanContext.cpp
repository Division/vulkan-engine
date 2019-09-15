#include "VulkanContext.h"
#include "VulkanUtils.h"
#include "VkObjects.h"
#include "CommandBufferManager.h"
#include "VulkanUploader.h"
#include "VulkanCaps.h"
#include "VulkanRenderPass.h"
#include "VulkanSwapchain.h"
#include "VulkanRenderTarget.h"

namespace core { namespace Device {

	const bool ENABLE_VALIDATION_LAYERS = true;

	VulkanContext::VulkanContext(GLFWwindow* window) : window(window)
	{
		CreateInstance();
		SetupDebugMessenger();

		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface!");
		}

		PickPhysicalDevice();
		CreateLogicalDevice();

		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = physicalDevice;
		allocatorInfo.device = device;
		vmaCreateAllocator(&allocatorInfo, &allocator);
	}

	void VulkanContext::initialize() // todo: remove
	{
		RecreateSwapChain();
		render_pass = std::make_unique<VulkanRenderPass>(VulkanRenderPassInitializer(swapchain->GetImageFormat()));

		auto render_target_initializer = VulkanRenderTargetInitializer(render_pass.get()).Swapchain(swapchain.get());
		main_render_target = std::make_unique<VulkanRenderTarget>(render_target_initializer);

		VulkanUtils::CreateDescriptorSetLayout(device, descriptor_set_layout);
		VulkanUtils::CreateGraphicsPipeline(device, swapchain->GetExtent(), descriptor_set_layout, render_pass->GetRenderPass(), pipelineLayout, graphicsPipeline);

		CreateCommandPool();

		command_buffer_manager = std::make_unique<CommandBufferManager>(caps::MAX_FRAMES_IN_FLIGHT);
		uploader = std::make_unique<VulkanUploader>();

		CreateSyncObjects();
	}

	VulkanContext::~VulkanContext()
	{

	}

	VkExtent2D VulkanContext::GetExtent() const 
	{ 
		return swapchain->GetExtent(); 
	
	}

	uint32_t VulkanContext::GetSwapchainImageCount() const 
	{ 
		return swapchain->GetImages().size(); 
	}

	VkFramebuffer VulkanContext::GetFramebuffer(uint32_t index) const
	{
		return main_render_target->GetFrame(index).framebuffer.get();
	}

	void VulkanContext::Cleanup()
	{
		command_buffer_manager = nullptr;
		vmaDestroyAllocator(allocator);
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL VulkanContext::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}

	void VulkanContext::RecreateSwapChain() {
		int width = 0, height = 0;
		while (width == 0 || height == 0) {
			glfwGetFramebufferSize(window, &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(device);
		swapchain = std::make_unique<VulkanSwapchain>(surface, width, height);
	}

	void VulkanContext::CreateInstance() {
		if (ENABLE_VALIDATION_LAYERS && !VulkanUtils::CheckValidationLayerSupport())
			throw std::runtime_error("validation layers requested, but not available!");

		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "VkEngine";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		auto extensions = VulkanUtils::GetRequiredExtensions(ENABLE_VALIDATION_LAYERS);
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		if (ENABLE_VALIDATION_LAYERS) 
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(VulkanUtils::VALIDATION_LAYERS.size());
			createInfo.ppEnabledLayerNames = VulkanUtils::VALIDATION_LAYERS.data();

			VulkanUtils::PopulateDebugMessengerCreateInfo(debugCreateInfo, VulkanContext::DebugCallback);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else 
		{
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
			throw std::runtime_error("failed to create instance!");
		}
	}

	void VulkanContext::SetupDebugMessenger() {
		if (!ENABLE_VALIDATION_LAYERS) return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		VulkanUtils::PopulateDebugMessengerCreateInfo(createInfo, VulkanContext::DebugCallback);

		if (VulkanUtils::CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
			throw std::runtime_error("failed to set up debug messenger!");
		}
	}

	void VulkanContext::PickPhysicalDevice() {
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

		if (deviceCount == 0) {
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		for (const auto& device : devices) {
			if (VulkanUtils::IsDeviceSuitable(device, surface)) {
				physicalDevice = device;
				break;
			}
		}

		if (physicalDevice == VK_NULL_HANDLE) {
			throw std::runtime_error("failed to find a suitable GPU!");
		}
	}

	void VulkanContext::CreateLogicalDevice() {
		VulkanUtils::QueueFamilyIndices indices = VulkanUtils::FindQueueFamilies(physicalDevice, surface);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures = {};
		deviceFeatures.samplerAnisotropy = VK_TRUE;

		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

		createInfo.pEnabledFeatures = &deviceFeatures;

		createInfo.enabledExtensionCount = static_cast<uint32_t>(VulkanUtils::DEVICE_EXTENSIONS.size());
		createInfo.ppEnabledExtensionNames = VulkanUtils::DEVICE_EXTENSIONS.data();

		if (ENABLE_VALIDATION_LAYERS) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(VulkanUtils::VALIDATION_LAYERS.size());
			createInfo.ppEnabledLayerNames = VulkanUtils::VALIDATION_LAYERS.data();
		}
		else {
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
			throw std::runtime_error("failed to create logical device!");
		}

		vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
	}

	void VulkanContext::CreateCommandPool() {
		VulkanUtils::QueueFamilyIndices queueFamilyIndices = VulkanUtils::FindQueueFamilies(physicalDevice, surface);

		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

		if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics command pool!");
		}
	}

	VulkanCommandBuffer* VulkanContext::BeginSingleTimeCommandBuffer()
	{
		auto* command_buffer = command_buffer_manager->GetDefaultCommandPool()->GetCommandBuffer();

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(command_buffer->GetCommandBuffer(), &beginInfo);

		return command_buffer;
	}

	void VulkanContext::EndSingleTimeCommandBuffer(VulkanCommandBuffer* command_buffer)
	{
		vkEndCommandBuffer(command_buffer->GetCommandBuffer());
		auto vk_command_buffer = command_buffer->GetCommandBuffer();

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &vk_command_buffer;

		vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue);
	}

    void VulkanContext::CreateSyncObjects() {
        imageAvailableSemaphores.resize(caps::MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(caps::MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(caps::MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < caps::MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
    }

	void VulkanContext::FrameRenderEnd()
	{
		currentFrame = (currentFrame + 1) % caps::MAX_FRAMES_IN_FLIGHT;
		command_buffer_manager->GetDefaultCommandPool()->NextFrame();
	}

} }