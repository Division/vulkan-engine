#pragma once

#include "CommonIncludes.h"

namespace core { namespace Device {

	class CommandBufferManager;
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
		CommandBufferManager* GetCommandBufferManager() const { return command_buffer_manager.get(); }
		VulkanUploader* GetUploader() const { return uploader.get(); }

		VkQueue GetGraphicsQueue() const { return graphicsQueue; }
		VkQueue GetPresentQueue() const { return presentQueue; }
		VulkanRenderPass* GetRenderPass() const { return render_pass.get(); };
		VkCommandPool GetCommandPool() const { return commandPool; };

		VulkanCommandBuffer* BeginSingleTimeCommandBuffer();
		void EndSingleTimeCommandBuffer(VulkanCommandBuffer* commandBuffer);

		VkFramebuffer GetFramebuffer(uint32_t index) const;
		VkExtent2D GetExtent() const;

		size_t GetCurrentFrame() const { return currentFrame; }
		uint32_t GetSwapchainImageCount() const;
		VkFence GetInFlightFence() const { return inFlightFences[currentFrame]; }
		VkSemaphore GetRenderFinishedSemaphore() const { return renderFinishedSemaphores[currentFrame]; }
		VkSemaphore GetImageAvailableSemaphore() const { return imageAvailableSemaphores[currentFrame]; }
		VulkanRenderState* GetRenderState();
		VulkanRenderTarget* GetMainRenderTarget() const { return main_render_target.get(); }

		VmaAllocator GetAllocator() { return allocator; }
		void AddFrameCommandBuffer(vk::CommandBuffer command_buffer);

		void Cleanup();

		void RecreateSwapChain();
		void Present();
	private:
		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

		void CreateInstance();
		void SetupDebugMessenger();
		void PickPhysicalDevice();
		void CreateLogicalDevice();
		void CreateCommandPool();
		void CreateSyncObjects();

	private:
		GLFWwindow* window;
		
		std::unique_ptr<CommandBufferManager> command_buffer_manager;
		std::unique_ptr<VulkanUploader> uploader;
		std::unique_ptr<VulkanSwapchain> swapchain;
		std::unique_ptr<VulkanRenderTarget> main_render_target;

		VkInstance instance;
		VkDebugUtilsMessengerEXT debugMessenger;
		VkSurfaceKHR surface;

		std::unique_ptr<VulkanRenderPass> render_pass;
		VkCommandPool commandPool;

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

		VmaAllocator allocator;

		size_t currentFrame = 0;
	};

} }