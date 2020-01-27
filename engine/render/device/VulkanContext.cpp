#include "VulkanContext.h"
#include "VulkanUtils.h"
#include "VkObjects.h"
#include "VulkanUploader.h"
#include "VulkanCaps.h"
#include "VulkanRenderPass.h"
#include "VulkanSwapchain.h"
#include "VulkanRenderTarget.h"
#include "VulkanRenderState.h"
#include "VulkanDescriptorCache.h"
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

		descriptor_cache = std::make_unique<VulkanDescriptorCache>(device);
	}

	void VulkanContext::initialize() // todo: remove
	{
		uploader = std::make_unique<VulkanUploader>();
		CreateSyncObjects();
	}

	VulkanContext::~VulkanContext()
	{
		vmaDestroyAllocator(allocator);
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

	void VulkanContext::AddFrameCommandBuffer(FrameCommandBufferData command_buffer)
	{
		frame_command_buffers.push_back(command_buffer);
	}

	void VulkanContext::Cleanup()
	{
		render_states.clear();
		uploader = nullptr;
		swapchain = nullptr;
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL VulkanContext::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}

	void VulkanContext::WindowResized()
	{
		framebuffer_resized = true;
	}

	void VulkanContext::RecreateSwapChain() 
	{
		int32_t width = 0;
		int32_t height = 0;
		glfwGetFramebufferSize(window, &width, &height);

		while (width == 0 || height == 0)
		{
			glfwWaitEvents();
			glfwGetFramebufferSize(window, &width, &height);
		}

		vkDeviceWaitIdle(device);
		swapchain = std::make_unique<VulkanSwapchain>(surface, width, height, swapchain.get());
		for (auto& callback : recreate_swapchain_callbacks)
			callback(width, height);
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

		graphics_queue = GetDevice().getQueue(indices.graphicsFamily.value(), 0);
		present_queue = GetDevice().getQueue(indices.presentFamily.value(), 0);
		compute_queue = GetDevice().getQueue(indices.compute_family.value(), 0);
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

	vk::Queue VulkanContext::GetQueue(PipelineBindPoint bind_point)
	{
		switch (bind_point)
		{
		case PipelineBindPoint::Graphics:
			return GetGraphicsQueue();
		
		case PipelineBindPoint::Compute:
			return GetComputeQueue();

		default:
			throw std::runtime_error("not supported");
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
		vk::Semaphore image_available_semaphone = GetImageAvailableSemaphore();
		vk::Semaphore render_finished_semaphore = GetRenderFinishedSemaphore();

		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(device, swapchain->GetSwapchain(), std::numeric_limits<uint64_t>::max(), image_available_semaphone, VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			RecreateSwapChain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		GetUploader()->ProcessUpload();

		for (int i = 0; i < frame_command_buffers.size(); i++)
		{
			bool is_last = i == frame_command_buffers.size() - 1;
			bool is_first = i == 0;

			auto& data = frame_command_buffers[i];
			vk::SubmitInfo submit_info;
			vk::PipelineStageFlags wait_mask;
			if (data.wait_semaphore)
			{
				submit_info.waitSemaphoreCount = 1;
				wait_mask = data.wait_flags;
				submit_info.pWaitDstStageMask = &wait_mask;
				submit_info.setPWaitSemaphores(&data.wait_semaphore);
			}

			if (is_first)
			{
				submit_info.waitSemaphoreCount = 1;
				wait_mask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
				submit_info.pWaitDstStageMask = &wait_mask;
				submit_info.setPWaitSemaphores(&image_available_semaphone);
			}

			auto signal_semaphore = is_last ? render_finished_semaphore : data.signal_semaphore;
			vk::Fence fence = is_last ? current_fence : nullptr;

			if (signal_semaphore)
			{
				submit_info.signalSemaphoreCount = 1;
				submit_info.setPSignalSemaphores(&signal_semaphore);
			}

			submit_info.commandBufferCount = 1;
			submit_info.setPCommandBuffers(&data.command_buffer);

			auto queue = GetQueue(data.queue);
			queue.submit(1, &submit_info, fence);
		}

		vk::SwapchainKHR swapChains[] = { swapchain->GetSwapchain() };
		vk::PresentInfoKHR presentInfo(1, &render_finished_semaphore, 1, swapChains, &imageIndex);
		auto present_result = GetPresentQueue().presentKHR(&presentInfo);

		//vkDeviceWaitIdle(device); ///// Uncomment to sync every frame

		if (present_result == vk::Result::eErrorOutOfDateKHR || present_result == vk::Result::eSuboptimalKHR || framebuffer_resized) {
			framebuffer_resized = false;
			RecreateSwapChain();
			currentFrame = 1; // todo: understand why it removes validation layer message
		}
		else if (present_result != vk::Result::eSuccess) {
			throw std::runtime_error("failed to present swap chain image!");
		}

		frame_command_buffers.clear();
		currentFrame = (currentFrame + 1) % caps::MAX_FRAMES_IN_FLIGHT;
		current_render_state = 0;
	}

	void VulkanContext::AddRecreateSwapchainCallback(RecreateSwapchainCallback callback)
	{
		recreate_swapchain_callbacks.push_back(callback);
	}

} }
