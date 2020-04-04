#pragma once

#include "CommonIncludes.h"
#include "Types.h"
#include "render/debug/Profiler.h"

namespace core { namespace Device {

	class VulkanCommandBuffer;
	class VulkanUploader;
	class VulkanRenderPass;
	class VulkanSwapchain;
	class VulkanRenderTarget;
	class VulkanRenderState;
	class VulkanDescriptorCache;
	
	struct SemaphoreList
	{
		std::vector<vk::Semaphore> semaphores;
		std::vector<vk::PipelineStageFlags> stage_flags;
	};

	struct FrameCommandBufferData
	{
		FrameCommandBufferData() = default;

		FrameCommandBufferData(
			vk::CommandBuffer command_buffer,
			vk::Semaphore signal_semaphore,
			SemaphoreList wait_semaphores,
			PipelineBindPoint queue) 
		: command_buffer(command_buffer)
		, signal_semaphore(signal_semaphore)
		, wait_semaphores(std::move(wait_semaphores))
		, queue(queue)
		{}

		vk::CommandBuffer command_buffer;
		vk::Semaphore signal_semaphore;
		SemaphoreList wait_semaphores;
		PipelineBindPoint queue;
	};

	class VulkanContext : public NonCopyable
	{
	public:
		typedef std::function<void(int32_t width, int32_t height)> RecreateSwapchainCallback;


		VulkanContext(GLFWwindow* window);
		virtual ~VulkanContext();
		void initialize();
	
		vk::Device GetDevice() const { return device; }
		vk::PhysicalDevice GetPhysicalDevice() const { return physicalDevice; }
		VulkanSwapchain* GetSwapchain() const { return swapchain.get(); }

		VkSurfaceKHR GetSurface() const { return surface; }
		VulkanUploader* GetUploader() const { return uploader.get(); }

		const auto& GetDeviceProps() const { return device_props; }
		const auto& GetMemoryProps() const { return memory_props; }

		const auto& GetProfilerTimings() const { return profiler_timings; };

		vk::Queue GetGraphicsQueue() const { return graphics_queue; }
		vk::Queue GetPresentQueue() const { return present_queue; }
		vk::Queue GetComputeQueue() const { return compute_queue; }
		vk::Queue GetQueue(PipelineBindPoint bind_point);
		uint32_t GetQueueFamilyIndex(PipelineBindPoint);

		size_t GetCurrentFrame() const { return currentFrame; }
		uint32_t GetSwapchainImageCount() const;
		VkFence GetInFlightFence() const { return inFlightFences[currentFrame]; }
		VkSemaphore GetRenderFinishedSemaphore() const { return renderFinishedSemaphores[currentFrame]; }
		VkSemaphore GetImageAvailableSemaphore() const { return imageAvailableSemaphores[currentFrame]; }
		VulkanRenderState* GetRenderState();
		VulkanDescriptorCache* GetDescriptorCache() const { return descriptor_cache.get(); }

		VmaAllocator GetAllocator() { return allocator; }
		void AddFrameCommandBuffer(FrameCommandBufferData command_buffer);

		void AddRecreateSwapchainCallback(RecreateSwapchainCallback callback);

		void Cleanup();

		void WindowResized();
		void RecreateSwapChain();
		void WaitForRenderFence();
		void Present();

		void BeginDebugMarker(VulkanCommandBuffer& command_buffer, const char* string);
		void InsertDebugMarker(VulkanCommandBuffer& command_buffer, const char* string);
		void EndDebugMarker(VulkanCommandBuffer& command_buffer);
		void AssignDebugName(uint64_t id, vk::DebugReportObjectTypeEXT type, const char* name);
	private:
		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

		void CreateInstance();
		void PickPhysicalDevice();
		void CreateLogicalDevice();
		void CreateSyncObjects();
		void SetupDebugMessenger();
		void SetupDebugMarker();
		void SetupDebugName();

	private:
		GLFWwindow* window;
		
		std::unique_ptr<VulkanUploader> uploader;
		std::unique_ptr<VulkanSwapchain> swapchain;
		std::unique_ptr<VulkanDescriptorCache> descriptor_cache;

		VkInstance instance;
		VkSurfaceKHR surface;

		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkDevice device;
		vk::PhysicalDeviceProperties device_props;
		vk::PhysicalDeviceMemoryProperties memory_props;

		vk::Queue graphics_queue;
		vk::Queue present_queue;
		vk::Queue compute_queue;

		uint32_t graphics_queue_index;
		uint32_t present_queue_index;
		uint32_t compute_queue_index;

		std::vector<std::unique_ptr<VulkanRenderState>> render_states;
		uint32_t current_render_state = 0;
		std::vector<VkSemaphore> imageAvailableSemaphores;
		std::vector<VkSemaphore> renderFinishedSemaphores;
		std::vector<VkFence> inFlightFences;
		std::vector<FrameCommandBufferData> frame_command_buffers;
		
		std::vector<RecreateSwapchainCallback> recreate_swapchain_callbacks;

		bool framebuffer_resized = false;
		size_t currentFrame = 0;
		VmaAllocator allocator;

		// Debug
		VkDebugUtilsMessengerEXT debug_messenger;
		PFN_vkDebugMarkerSetObjectNameEXT debug_object_name_callback = nullptr;
		PFN_vkCmdDebugMarkerBeginEXT debug_marker_begin_callback = nullptr;
		PFN_vkCmdDebugMarkerEndEXT debug_marker_end_callback = nullptr;
		PFN_vkCmdDebugMarkerInsertEXT debug_marker_insert_callback = nullptr;
		core::render::profiler::ProfilerTimings profiler_timings;
	};

} }