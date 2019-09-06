#pragma once

#include "CommonIncludes.h"

namespace core { namespace Device {

	class CommandBufferManager;
	class VulkanCommandBuffer;
	class VulkanUploader;

	class VulkanContext
	{
	public:
		static const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

		VulkanContext(GLFWwindow* window);
		virtual ~VulkanContext();
	
		VkDevice GetDevice() const { return device; }
		VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice; }
		VkPipelineLayout GetPipelineLayout() const { return pipelineLayout; }
		VkPipeline GetPipeline() const { return graphicsPipeline; }
		VkSwapchainKHR GetSwapchain() const { return swapChain; }
		VkSurfaceKHR GetSurface() const { return surface; }
		CommandBufferManager* GetCommandBufferManager() const { return command_buffer_manager.get(); }
		VulkanUploader* GetUploader() const { return uploader.get(); }

		VkQueue GetGraphicsQueue() const { return graphicsQueue; }
		VkQueue GetPresentQueue() const { return presentQueue; }
		VkRenderPass GetRenderPass() { return render_pass; }
		VkDescriptorSetLayout GetDescriptorSetLayout() const { return descriptor_set_layout; };
		VkCommandPool GetCommandPool() const { return commandPool; };

		VulkanCommandBuffer* BeginSingleTimeCommandBuffer();
		void EndSingleTimeCommandBuffer(VulkanCommandBuffer* commandBuffer);

		VkFramebuffer GetFramebuffer(uint32_t index) const { return swapChainFramebuffers[index]; };
		VkExtent2D GetExtent() const { return swapChainExtent; }

		uint32_t GetSwapchainImageCount() const { return swapChainImages.size(); }
		VkFence GetInFlightFence() const { return inFlightFences[currentFrame]; }
		VkSemaphore GetRenderFinishedSemaphore() const { return renderFinishedSemaphores[currentFrame]; }
		VkSemaphore GetImageAvailableSemaphore() const { return imageAvailableSemaphores[currentFrame]; }

		VmaAllocator GetAllocator() { return allocator; }
		
		void Cleanup();

		void RecreateSwapChain();
		void FrameRenderEnd();
	private:
		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

		void CreateInstance();
		void SetupDebugMessenger();
		void PickPhysicalDevice();
		void CreateLogicalDevice();
		void CreateSwapChain();
		void CreateFramebuffers();
		void CreateCommandPool();
		void CreateSyncObjects();

	private:
		GLFWwindow* window;
		
		std::unique_ptr<CommandBufferManager> command_buffer_manager;
		std::unique_ptr<VulkanUploader> uploader;

		VkInstance instance;
		VkDebugUtilsMessengerEXT debugMessenger;
		VkSurfaceKHR surface;
		VkSwapchainKHR swapChain;

		VkRenderPass render_pass;
		VkDescriptorSetLayout descriptor_set_layout;
		VkCommandPool commandPool;

		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkDevice device;
		VkPipelineLayout pipelineLayout;
		VkPipeline graphicsPipeline;

		VkQueue graphicsQueue;
		VkQueue presentQueue;

		std::vector<VkImage> swapChainImages;
		VkFormat swapChainImageFormat;
		VkExtent2D swapChainExtent;
		std::vector<VkImageView> swapChainImageViews;
		std::vector<VkFramebuffer> swapChainFramebuffers;

		std::vector<VkSemaphore> imageAvailableSemaphores;
		std::vector<VkSemaphore> renderFinishedSemaphores;
		std::vector<VkFence> inFlightFences;

		VmaAllocator allocator;

		size_t currentFrame = 0;
	};

} }