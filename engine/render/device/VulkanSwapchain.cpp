#include "VulkanSwapchain.h"
#include "Engine.h"
#include "VulkanContext.h"
#include "VulkanUtils.h"
#include "VulkanRenderPass.h"
#include "VulkanRenderTarget.h"

namespace Device {

	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> present_modes;
	};

	SwapChainSupportDetails QuerySwapChainSupport(vk::PhysicalDevice device, VkSurfaceKHR surface) {
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

		if (presentModeCount != 0) {
			details.present_modes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.present_modes.data());
		}

		return details;
	}

	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, int width, int height) {
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max() && capabilities.currentExtent.width > 0) {
			return capabilities.currentExtent;
		}
		else {
			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

			return actualExtent;
		}
	}

	VulkanSwapchain::VulkanSwapchain(vk::SurfaceKHR surface, uint32_t width, uint32_t height, VulkanSwapchain* old_swapchain)
		: surface(surface)
		, width(width)
		, height(height)
	{
		auto physical_device = Engine::GetVulkanContext()->GetPhysicalDevice();
		auto device = Engine::GetVulkanDevice();
		auto support = QuerySwapChainSupport(physical_device, surface );

		VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(support.formats);
		vk::PresentModeKHR presentMode = vk::PresentModeKHR(ChooseSwapPresentMode(support.present_modes));
		VkExtent2D extent = ChooseSwapExtent(support.capabilities, width, height);

		uint32_t image_count = std::max(support.capabilities.minImageCount, 2u);
		if (support.capabilities.maxImageCount > 0 && image_count > support.capabilities.maxImageCount) 
			image_count = support.capabilities.maxImageCount;

		VulkanUtils::QueueFamilyIndices indices = VulkanUtils::FindQueueFamilies(physical_device, surface);
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		vk::SharingMode image_sharing_mode;
		uint32_t* queueFamilyIndicesPointer = nullptr;
		uint32_t queueFamilyIndexCount = 0;

		if (indices.graphicsFamily != indices.presentFamily) {
			image_sharing_mode = vk::SharingMode::eConcurrent;
			queueFamilyIndexCount = 2;
			queueFamilyIndicesPointer = queueFamilyIndices;
		}
		else {
			image_sharing_mode = vk::SharingMode::eExclusive;
		}

		vk::ImageUsageFlags image_usage_flags = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;
		vk::SurfaceTransformFlagBitsKHR pre_transform = vk::SurfaceTransformFlagBitsKHR(support.capabilities.currentTransform);
		vk::CompositeAlphaFlagBitsKHR composite_alpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
		image_format = vk::Format(surfaceFormat.format);

		vk::SwapchainCreateInfoKHR create_info(
			{},
			surface,
			image_count,
			image_format,
			vk::ColorSpaceKHR(surfaceFormat.colorSpace),
			extent,
			1,
			image_usage_flags, image_sharing_mode,
			queueFamilyIndexCount, queueFamilyIndicesPointer,
			pre_transform,
			composite_alpha,
			presentMode,
			VK_TRUE,
			old_swapchain ? old_swapchain->GetSwapchain() : vk::SwapchainKHR()
		);

		swapchain = device.createSwapchainKHRUnique(create_info);
		Engine::Get()->GetContext()->AssignDebugName((uint64_t)(VkSwapchainKHR)swapchain.get(), vk::DebugReportObjectTypeEXT::eSwapchainKHR, "Swapchain");
		width = extent.width;
		height = extent.height;

		images = device.getSwapchainImagesKHR(swapchain.get());
		assert(images.size() == 2); // TODO: handle swapchain images properly
		color_attachments[0] = std::make_unique<VulkanRenderTargetAttachment>(this, 0);
		color_attachments[1] = std::make_unique<VulkanRenderTargetAttachment>(this, 1);

		for (int i = 0; i < images.size(); i++)
			Engine::Get()->GetContext()->AssignDebugName((uint64_t)(VkImage)images[i], vk::DebugReportObjectTypeEXT::eSwapchainKHR, ("Swapchain Image " + std::to_string(i + 1)).c_str());
	}

}