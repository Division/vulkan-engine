#include "VulkanContext.h"
#include "VulkanUtils.h"
#include "VkObjects.h"
#include "VulkanUploader.h"
#include "VulkanCaps.h"
#include "VulkanRenderPass.h"
#include "VulkanSwapchain.h"
#include "VulkanRenderTarget.h"
#include "VulkanRenderState.h"
#include <mutex>

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

		uploader = std::make_unique<VulkanUploader>();

		CreateSyncObjects();
	}

	VulkanContext::~VulkanContext()
	{

	}

	VulkanRenderState* VulkanContext::GetRenderState()
	{
		static std::mutex mutex;
		std::lock_guard<std::mutex> lock(mutex);

		VulkanRenderState* result = nullptr;
		if (current_render_state < render_states.size())
			result = render_states[current_render_state].get();
		else
		{
			render_states.push_back(std::make_unique<VulkanRenderState>());
			result = render_states.back().get();
		}

		current_render_state += 1;
		return result;
	}

	uint32_t VulkanContext::GetSwapchainImageCount() const 
	{ 
		return swapchain->GetImages().size(); 
	}

	void VulkanContext::AddFrameCommandBuffer(vk::CommandBuffer command_buffer)
	{
		frame_command_buffers.push_back(command_buffer);
	}

	void VulkanContext::Cleanup()
	{
		vmaDestroyAllocator(allocator);
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL VulkanContext::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}

	void VulkanContext::RecreateSwapChain() 
	{
		int width = 0, height = 0;
		while (width == 0 || height == 0) {
			glfwGetFramebufferSize(window, &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(device);
		swapchain = std::make_unique<VulkanSwapchain>(surface, width, height);
	}

	void VulkanContext::CreateInstance() 
	{
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

	void VulkanContext::SetupDebugMessenger() 
	{
		if (!ENABLE_VALIDATION_LAYERS) return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		VulkanUtils::PopulateDebugMessengerCreateInfo(createInfo, VulkanContext::DebugCallback);

		if (VulkanUtils::CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
			throw std::runtime_error("failed to set up debug messenger!");
		}
	}

	void VulkanContext::PickPhysicalDevice() 
	{
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

	void VulkanContext::CreateLogicalDevice() 
	{
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

    void VulkanContext::CreateSyncObjects() 
	{
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

	void VulkanContext::WaitForRenderFence()
	{
		vk::Fence current_fence = GetInFlightFence(); 
		GetDevice().waitForFences(1, &current_fence, VK_TRUE, std::numeric_limits<uint64_t>::max());
		GetDevice().resetFences(1, &current_fence);
	}

	void VulkanContext::Present()
	{
		auto vk_swapchain = GetSwapchain();
		VkFence current_fence = GetInFlightFence();
		VkSemaphore image_available_semaphone = GetImageAvailableSemaphore();
		VkSemaphore render_finished_semaphore = GetRenderFinishedSemaphore();

		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(device, swapchain->GetSwapchain(), std::numeric_limits<uint64_t>::max(), image_available_semaphone, VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			throw new std::runtime_error("recreate not supported");
			//recreateSwapChain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		GetUploader()->ProcessUpload();

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { image_available_semaphone };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = frame_command_buffers.size();
		submitInfo.pCommandBuffers = frame_command_buffers.data();

		VkSemaphore signalSemaphores[] = { render_finished_semaphore };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		// Queue waits for the image_available_semaphone which is signaled after vkAcquireNextImageKHR succeeds
		if (vkQueueSubmit(GetGraphicsQueue(), 1, &submitInfo, current_fence) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}


		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { swapchain->GetSwapchain() };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;

		presentInfo.pImageIndices = &imageIndex;

		result = vkQueuePresentKHR(GetPresentQueue(), &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR /*|| framebufferResized*/) {
			throw new std::runtime_error("recreate not supported");
			//framebufferResized = false;
			//recreateSwapChain();
		}
		else if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image!");
		}

		frame_command_buffers.clear();
		currentFrame = (currentFrame + 1) % caps::MAX_FRAMES_IN_FLIGHT;
		current_render_state = 0;
	}

} }