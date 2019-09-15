#pragma once

#include "CommonIncludes.h"

namespace core { namespace Device {

	class CommandBufferManager;
	class VulkanCommandBuffer;
	class VulkanUploader;
	class VulkanRenderPass;
	class VulkanSwapchain;
	class VulkanRenderTarget;

	class VulkanContext : public NonCopyable
	{
	public:
		VulkanContext(GLFWwindow* window);
		virtual ~VulkanContext();
		void initialize();
	
		vk::Device GetDevice() const { return device; }
		vk::PhysicalDevice GetPhysicalDevice() const { return physicalDevice; }
		VkPipelineLayout GetPipelineLayout() const { return pipelineLayout; }
		VkPipeline GetPipeline() const { return graphicsPipeline; }
		VulkanSwapchain* GetSwapchain() const { return swapchain.get(); }
		VkSurfaceKHR GetSurface() const { return surface; }
		CommandBufferManager* GetCommandBufferManager() const { return command_buffer_manager.get(); }
		VulkanUploader* GetUploader() const { return uploader.get(); }

		VkQueue GetGraphicsQueue() const { return graphicsQueue; }
		VkQueue GetPresentQueue() const { return presentQueue; }
		VulkanRenderPass* GetRenderPass() const { return render_pass.get(); };
		VkDescriptorSetLayout GetDescriptorSetLayout() const { return descriptor_set_layout; };
		VkCommandPool GetCommandPool() const { return commandPool; };

		VulkanCommandBuffer* BeginSingleTimeCommandBuffer();
		void EndSingleTimeCommandBuffer(VulkanCommandBuffer* commandBuffer);

		VkFramebuffer GetFramebuffer(uint32_t index) const;
		VkExtent2D GetExtent() const;

		uint32_t GetSwapchainImageCount() const;
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
		VkDescriptorSetLayout descriptor_set_layout;
		VkCommandPool commandPool;

		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkDevice device;
		VkPipelineLayout pipelineLayout;
		VkPipeline graphicsPipeline;

		VkQueue graphicsQueue;
		VkQueue presentQueue;

		std::vector<VkSemaphore> imageAvailableSemaphores;
		std::vector<VkSemaphore> renderFinishedSemaphores;
		std::vector<VkFence> inFlightFences;

		VmaAllocator allocator;

		size_t currentFrame = 0;
	};

} }