#pragma once

#include "CommonIncludes.h"

namespace core { namespace Device {

	class VulkanCommandBuffer;
	class VulkanUploader;
	class VulkanRenderPass;
	class VulkanSwapchain;
	class VulkanRenderTarget;
	class VulkanRenderState;

	class VulkanContext : public NonCopyable
	{
	public:
		VulkanContext(GLFWwindow* window);
		virtual ~VulkanContext();
		void initialize();
	
		vk::Device GetDevice() const { return device; }
		vk::PhysicalDevice GetPhysicalDevice() const { return physicalDevice; }
		VulkanSwapchain* GetSwapchain() const { return swapchain.get(); }

		VkSurfaceKHR GetSurface() const { return surface; }
		VulkanUploader* GetUploader() const { return uploader.get(); }

		VkQueue GetGraphicsQueue() const { return graphicsQueue; }
		VkQueue GetPresentQueue() const { return presentQueue; }

		size_t GetCurrentFrame() const { return currentFrame; }
		uint32_t GetSwapchainImageCount() const;
		VkFence GetInFlightFence() const { return inFlightFences[currentFrame]; }
		VkSemaphore GetRenderFinishedSemaphore() const { return renderFinishedSemaphores[currentFrame]; }
		VkSemaphore GetImageAvailableSemaphore() const { return imageAvailableSemaphores[currentFrame]; }
		VulkanRenderState* GetRenderState();

		VmaAllocator GetAllocator() { return allocator; }
		void AddFrameCommandBuffer(vk::CommandBuffer command_buffer);

		void Cleanup();

		void WindowResized();
		void RecreateSwapChain();
		void WaitForRenderFence();
		void Present();
	private:
		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

		void CreateInstance();
		void SetupDebugMessenger();
		void PickPhysicalDevice();
		void CreateLogicalDevice();
		void CreateSyncObjects();

	private:
		GLFWwindow* window;
		
		std::unique_ptr<VulkanUploader> uploader;
		std::unique_ptr<VulkanSwapchain> swapchain;

		VkInstance instance;
		VkDebugUtilsMessengerEXT debugMessenger;
		VkSurfaceKHR surface;

		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkDevice device;

		VkQueue graphicsQueue;
		VkQueue presentQueue;

		std::vector<std::unique_ptr<VulkanRenderState>> render_states;
		uint32_t current_render_state = 0;
		std::vector<VkSemaphore> imageAvailableSemaphores;
		std::vector<VkSemaphore> renderFinishedSemaphores;
		std::vector<VkFence> inFlightFences;
		std::vector<VkCommandBuffer> frame_command_buffers;

		bool framebuffer_resized = false;
		size_t currentFrame = 0;
		VmaAllocator allocator;
	};

} }