#pragma once

#include "CommonIncludes.h"

namespace Device {

	class VulkanContext;
	class VulkanBuffer;
	class VulkanRenderState;
}

namespace Device::VulkanUtils {

	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;
		std::optional<uint32_t> compute_family;

		bool IsComplete() {
			return graphicsFamily.has_value() && presentFamily.has_value() && compute_family.has_value();
		}
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	extern const std::vector<const char*> VALIDATION_LAYERS;

	const std::vector<const char*> GetDeviceExtensions(bool validation_layers);
	bool CheckValidationLayerSupport();
	std::vector<const char*> GetRequiredExtensions(bool enable_validation_layers);
	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo, PFN_vkDebugUtilsMessengerCallbackEXT callback);
	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

	vk::BufferMemoryBarrier BufferTransition(VulkanBuffer& buffer, vk::AccessFlags before, vk::AccessFlags after);

	void FullBarrier(VulkanRenderState& state);
}