#include "VulkanContext.h"
#include "VulkanUtils.h"
#include "VulkanUploader.h"
#include "VulkanCaps.h"
#include "VulkanRenderPass.h"
#include "VulkanSwapchain.h"
#include "VulkanRenderTarget.h"
#include "VulkanRenderState.h"
#include "VulkanDescriptorCache.h"
#include <mutex>

namespace Device {

	using namespace render;

	const bool ENABLE_VALIDATION_LAYERS = true;

	FrameCommandBufferData::FrameCommandBufferData(
		const VulkanRenderState& state,
		const RpsCommandBatch& batch,
		const uint32_t* pWaitSemaphoreIndices)
		:
		command_buffer(state.getRecorderCommandBuffer()),
		bindPoint(state.getPipelineBindPoint()),
		rpsBatch(batch)
	{
		waitSemaphoreIndices.resize(batch.numWaitFences);
		for (uint32_t i = 0; i < batch.numWaitFences; i++)
		{
			waitSemaphoreIndices[i] = *(pWaitSemaphoreIndices + i);
		}
	};


	VulkanContext::VulkanContext(GLFWwindow* window) : window(window)
	{
		OPTICK_EVENT();
		CreateInstance();

		if (ENABLE_VALIDATION_LAYERS)
		{
			SetupDebugMessenger();
			SetupDebugMarker();
			SetupDebugName();
		}

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

		CreateRPSDevice();
	}

	void VulkanContext::initialize() // todo: remove
	{
		uploader = std::make_unique<VulkanUploader>();
		CreateSyncObjects();
		profiler::Initialize();
	}

	VulkanContext::~VulkanContext()
	{
		profiler::Deinitialize();
		descriptor_cache = nullptr;
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

	void VulkanContext::BeginDebugMarker(vk::CommandBuffer command_buffer, const char* string)
	{
		if (ENABLE_VALIDATION_LAYERS)
		{
			vk::DebugMarkerMarkerInfoEXT info;
			info.setPMarkerName(string);
			debug_marker_begin_callback(command_buffer, (VkDebugMarkerMarkerInfoEXT*)&info);
		}
	}

	void VulkanContext::InsertDebugMarker(vk::CommandBuffer command_buffer, const char* string)
	{
		if (ENABLE_VALIDATION_LAYERS)
		{
			vk::DebugMarkerMarkerInfoEXT info;
			info.setPMarkerName(string);
			debug_marker_insert_callback(command_buffer, (VkDebugMarkerMarkerInfoEXT*)&info);
		}
	}

	void VulkanContext::EndDebugMarker(vk::CommandBuffer command_buffer)
	{
		if (ENABLE_VALIDATION_LAYERS)
		{
			debug_marker_end_callback(command_buffer);
		}
	}

	void VulkanContext::AssignDebugName(uint64_t id, vk::DebugReportObjectTypeEXT type, const char* name)
	{
		if (debug_object_name_callback)
		{
			vk::DebugMarkerObjectNameInfoEXT name_info(type, id, name);
			debug_object_name_callback(device, (VkDebugMarkerObjectNameInfoEXT*)&name_info);
		}
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

	VKAPI_ATTR VkBool32 VKAPI_CALL VulkanContext::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) 
	{
		if (pCallbackData->messageIdNumber == 0x609a13b) // UNASSIGNED-CoreValidation-Shader-OutputNotConsumed
			return VK_FALSE;

		if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
		{
			//std::cerr << "[VK] GENERAL ";
			return VK_FALSE;
		}
		else if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
			std::cerr << "[VK] VALIDATION ";
		else if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
			std::cerr << "[VK] PERFOMANCE ";

		if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
			std::cerr << "ERROR: ";
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
			std::cerr << "WARNING: ";
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
			std::cerr << "INFO: ";
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
			std::cerr << "VERBOSE: ";

		std::cerr << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}

	void VulkanContext::WindowResized()
	{
		framebuffer_resized = true;
	}

	void VulkanContext::RecreateSwapChain() 
	{
		OPTICK_EVENT();
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
		OPTICK_EVENT();
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
		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		VulkanUtils::PopulateDebugMessengerCreateInfo(createInfo, VulkanContext::DebugCallback);

		if (VulkanUtils::CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debug_messenger) != VK_SUCCESS) {
			throw std::runtime_error("failed to set up debug messenger!");
		}
	}

	void VulkanContext::SetupDebugMarker()
	{
		debug_marker_begin_callback = (PFN_vkCmdDebugMarkerBeginEXT)vkGetInstanceProcAddr(instance, "vkCmdDebugMarkerBeginEXT");
		debug_marker_end_callback = (PFN_vkCmdDebugMarkerEndEXT)vkGetInstanceProcAddr(instance, "vkCmdDebugMarkerEndEXT");
		debug_marker_insert_callback = (PFN_vkCmdDebugMarkerInsertEXT)vkGetInstanceProcAddr(instance, "vkCmdDebugMarkerInsertEXT");
	}

	void VulkanContext::SetupDebugName()
	{
		debug_object_name_callback = (PFN_vkDebugMarkerSetObjectNameEXT)vkGetInstanceProcAddr(instance, "vkDebugMarkerSetObjectNameEXT");
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
		else
		{
			vk::PhysicalDevice physical_device(physicalDevice);
			device_props = physical_device.getProperties();
			memory_props = physical_device.getMemoryProperties();
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

		VkPhysicalDeviceShaderDemoteToHelperInvocationFeaturesEXT demote_to_helper = {};
		demote_to_helper.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DEMOTE_TO_HELPER_INVOCATION_FEATURES_EXT;
		demote_to_helper.pNext = nullptr;
		demote_to_helper.shaderDemoteToHelperInvocation = true;

		VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
		deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		deviceFeatures2.pNext = &demote_to_helper;

		VkPhysicalDeviceFeatures& deviceFeatures = deviceFeatures2.features;
		deviceFeatures = {};
		deviceFeatures.fillModeNonSolid = true; // TODO: debug only?
		deviceFeatures.samplerAnisotropy = VK_TRUE;

		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pNext = &deviceFeatures2;

		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();


		auto device_extensions = VulkanUtils::GetDeviceExtensions(ENABLE_VALIDATION_LAYERS);
		createInfo.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
		createInfo.ppEnabledExtensionNames = device_extensions.data();

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

		graphics_queue_index = indices.graphicsFamily.value();
		graphics_queue = GetDevice().getQueue(graphics_queue_index, 0);
		present_queue_index = indices.presentFamily.value();
		present_queue = GetDevice().getQueue(present_queue_index, 0);
		// Don't support async compute for now
		compute_queue_index = graphics_queue_index;// indices.compute_family.value();
		compute_queue = GetDevice().getQueue(compute_queue_index, 0);

		AssignDebugName((uint64_t)(VkQueue)graphics_queue, vk::DebugReportObjectTypeEXT::eQueue, "Graphics Queue");
		//AssignDebugName((uint64_t)(VkQueue)compute_queue, vk::DebugReportObjectTypeEXT::eQueue, "Compute Queue");
		AssignDebugName((uint64_t)(VkQueue)present_queue, vk::DebugReportObjectTypeEXT::eQueue, "Present Queue");
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
	
	
	static void* CountedMalloc(void* pContext, size_t size, size_t alignment)
	{
		return _aligned_malloc(size, alignment);
	}

	static void CountedFree(void* pContext, void* ptr)
	{
		_aligned_free(ptr);
	}

	static void* CountedRealloc(
		void* pContext, void* oldBuffer, size_t oldSize, size_t newSize, size_t alignment)
	{
		void* pNewBuffer = oldBuffer;

		if (newSize > oldSize)
		{
			pNewBuffer = CountedMalloc(pContext, newSize, alignment);
			if (oldBuffer)
			{
				memcpy(pNewBuffer, oldBuffer, std::min(oldSize, newSize));
				CountedFree(pContext, oldBuffer);
			}
		}

		return pNewBuffer;
	}
	
	static void PrintToStdErr(void* pCtx, const char* formatString, ...)
	{
		va_list args;
		va_start(args, formatString);
#if _MSC_VER
		vfprintf_s(stderr, formatString, args);
#else
		vfprintf(stderr, formatString, args);
#endif
		va_end(args);

		fflush(stderr);

		// TODO: Currently rpsTestPrintDebugString only implemented for WIN32.
#if defined(_MSC_VER) && defined(_WIN32)
		char buf[4096];

		va_start(args, formatString);
		vsprintf_s(buf, formatString, args);
		va_end(args);
		std::cout << buf;
#endif
	}


	void VulkanContext::CreateRPSDevice()
	{
		RpsDeviceCreateInfo createInfo = {};
		createInfo.allocator.pfnAlloc = CountedMalloc;
		createInfo.allocator.pfnFree  = CountedFree;
		createInfo.allocator.pfnRealloc = CountedRealloc;
		createInfo.printer.pfnPrintf  = PrintToStdErr;
        RpsVKRuntimeDeviceCreateInfo vkRuntimeDeviceCreateInfo = {};
        vkRuntimeDeviceCreateInfo.pDeviceCreateInfo            = &createInfo;
        vkRuntimeDeviceCreateInfo.hVkDevice                    = GetDevice();
        vkRuntimeDeviceCreateInfo.hVkPhysicalDevice            = GetPhysicalDevice();
        vkRuntimeDeviceCreateInfo.flags                        = RPS_VK_RUNTIME_FLAG_DONT_FLIP_VIEWPORT;

        RpsRuntimeDeviceCreateInfo runtimeDeviceCreateInfo     = {};
        runtimeDeviceCreateInfo.pUserContext                   = this;
        //runtimeDeviceCreateInfo.callbacks.pfnRecordDebugMarker = &RecordDebugMarker;
        //runtimeDeviceCreateInfo.callbacks.pfnSetDebugName      = &SetDebugName;

        vkRuntimeDeviceCreateInfo.pRuntimeCreateInfo = &runtimeDeviceCreateInfo;

        RpsResult result = rpsVKRuntimeDeviceCreate(&vkRuntimeDeviceCreateInfo, &rpsDevice);
		if (result != RPS_OK)
		{
			throw std::runtime_error("Failed creating RpsDevice");
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

	uint32_t VulkanContext::GetQueueFamilyIndex(PipelineBindPoint bind_point)
	{
		switch (bind_point)
		{
		case PipelineBindPoint::Graphics:
			return graphics_queue_index;

		case PipelineBindPoint::Compute:
			return compute_queue_index;

		default:
			throw std::runtime_error("not supported");
		}
	}

	void VulkanContext::WaitForRenderFence()
	{
		vk::Fence current_fence = GetInFlightFence(); 
		GetDevice().waitForFences(1, &current_fence, VK_TRUE, std::numeric_limits<uint64_t>::max());
		GetDevice().resetFences(1, &current_fence);
		descriptor_cache->ResetFrameDescriptors();
	}

	void VulkanContext::Present()
	{
		OPTICK_EVENT();

		auto vk_swapchain = GetSwapchain();
		VkFence current_fence = GetInFlightFence();
		vk::Semaphore image_available_semaphone = GetImageAvailableSemaphore();
		vk::Semaphore render_finished_semaphore = GetRenderFinishedSemaphore();

		VkResult result = vkAcquireNextImageKHR(device, swapchain->GetSwapchain(), std::numeric_limits<uint64_t>::max(), image_available_semaphone, VK_NULL_HANDLE, &swapchainImageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			RecreateSwapChain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		profiler::GetProfilerTimings(profiler_timings);

		GetUploader()->ProcessUpload();
		vk::PipelineStageFlags color_wait_mask = vk::PipelineStageFlagBits::eColorAttachmentOutput;

		utils::SmallVector<vk::Semaphore, 32> waitSemaphores;

		for (int i = 0; i < frame_command_buffers.size(); i++)
		{
			bool is_last = i == frame_command_buffers.size() - 1;
			bool is_first = i == 0;

			auto& data = frame_command_buffers[i];

			const vk::PipelineStageFlags submitWaitStage = vk::PipelineStageFlagBits::eBottomOfPipe;

			waitSemaphores.clear();

			// Wait for swapchain
			if (is_first)
			{
				waitSemaphores.push_back(image_available_semaphone);
			}

			for (uint32_t i = 0; i < data.waitSemaphoreIndices.size(); i++)
			{
				waitSemaphores.push_back(queueSemaphores[data.waitSemaphoreIndices[i]]);
			}

			vk::Fence submitFence = VK_NULL_HANDLE;

			utils::SmallVector<vk::Semaphore, 2> signalSemaphores;

			if (is_last)
			{
				signalSemaphores.push_back(render_finished_semaphore);
				submitFence = current_fence;
			}

			if (data.rpsBatch.signalFenceIndex != UINT32_MAX)
			{
				signalSemaphores.push_back(queueSemaphores[data.rpsBatch.signalFenceIndex]);
			}

			vk::SubmitInfo submitInfo = {};
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &data.command_buffer;
			submitInfo.pWaitSemaphores = waitSemaphores.data();
			submitInfo.waitSemaphoreCount = (uint32_t)waitSemaphores.size();
			submitInfo.pSignalSemaphores = signalSemaphores.data();
			submitInfo.signalSemaphoreCount = (uint32_t)signalSemaphores.size();
			submitInfo.pWaitDstStageMask = &submitWaitStage;

			auto queue = GetQueue(data.bindPoint);
			queue.submit(1, &submitInfo, submitFence);
		}

		vk::SwapchainKHR swapChains[] = { swapchain->GetSwapchain() };
		vk::PresentInfoKHR presentInfo(1, &render_finished_semaphore, 1, swapChains, &swapchainImageIndex);
		auto present_result = GetPresentQueue().presentKHR(&presentInfo);

		///////
		//vkDeviceWaitIdle(device); ///// Uncomment to sync every frame
		///////

		if (present_result == vk::Result::eErrorOutOfDateKHR || present_result == vk::Result::eSuboptimalKHR || framebuffer_resized) {
			framebuffer_resized = false;
			RecreateSwapChain();
			currentFrame = 1; // todo: understand why it removes validation layer message
		}
		else if (present_result != vk::Result::eSuccess) {
			throw std::runtime_error("failed to present swap chain image!");
		}

		frame_command_buffers.clear();
		currentFrame = currentFrame + 1;
		current_render_state = 0;

		profiler::Update();
	}

	void VulkanContext::AddRecreateSwapchainCallback(RecreateSwapchainCallback callback)
	{
		recreate_swapchain_callbacks.push_back(callback);
	}

    void VulkanContext::ReserveSemaphores(uint32_t numSyncs)
    {
        const uint32_t oldSize = uint32_t(queueSemaphores.size());
        if (numSyncs > oldSize)
        {
            queueSemaphores.resize(numSyncs, VK_NULL_HANDLE);
        }

        VkSemaphoreCreateInfo semaphoreCI = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

        for (size_t i = oldSize; i < numSyncs; i++)
        {
			auto result = vkCreateSemaphore(device, &semaphoreCI, nullptr, &queueSemaphores[i]);
			assert(result == VK_SUCCESS);
        }
    }
}
