#pragma once

#include "CommonIncludes.h"
#include "render/DataStructures.h"

namespace core { namespace Device {

	class VulkanContext;

} }

namespace core { namespace Device { namespace VulkanUtils {

	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete() {
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	extern const std::vector<const char*> VALIDATION_LAYERS;
	extern const std::vector<const char*> DEVICE_EXTENSIONS;

	std::vector<char> ReadFile(const std::string& filename);
	VkShaderModule CreateShaderModule(VkDevice device, const std::vector<char>& code);

	bool CheckValidationLayerSupport();
	std::vector<const char*> GetRequiredExtensions(bool enable_validation_layers);
	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo, PFN_vkDebugUtilsMessengerCallbackEXT callback);
	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
	
	void CreateRenderPass(VkDevice device, VkFormat swapChainImageFormat, VkRenderPass& out_render_pass);
	void CreateDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout& out_descriptor_set_layout);

	void CreateGraphicsPipeline(
		VkDevice device,
		VkExtent2D swapChainExtent,
		VkDescriptorSetLayout descriptorSetLayout,
		VkRenderPass renderPass,
		VkPipelineLayout& out_pipelineLayout,
		VkPipeline& out_pipeline
	);

	uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
	void CreateBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void CopyBufferToImage(VulkanContext& context, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void TransitionImageLayout(VulkanContext& context, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void CopyBuffer(VulkanContext& context, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
} } }