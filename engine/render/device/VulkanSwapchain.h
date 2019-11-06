#pragma once 

#include "CommonIncludes.h"
#include "Types.h"

namespace core { namespace Device {

	class VulkanRenderPass;
	class VulkanRenderTarget;
	class VulkanRenderTargetAttachment;

	class VulkanSwapchain : NonCopyable
	{
	public:
		VulkanSwapchain(vk::SurfaceKHR surface, uint32_t width, uint32_t height, VulkanSwapchain* old_swapchain = nullptr);

		vk::SwapchainKHR GetSwapchain() const { return swapchain.get(); }
		Format GetImageFormat() const { return (Format)image_format; }
		vk::Extent2D GetExtent() const { return vk::Extent2D(width, height); }
		uint32_t GetWidth() const { return width; }
		uint32_t GetHeight() const { return height; }
		const std::vector<vk::Image>& GetImages() const { return images; }
		VulkanRenderTargetAttachment* GetColorAttachment() const { return color_attachment.get(); }
		VulkanRenderTargetAttachment* GetDepthAttachment() const { return depth_attachment.get(); }

	private:
		vk::UniqueSwapchainKHR swapchain;
		vk::SurfaceKHR surface;
		uint32_t width;
		uint32_t height;
		std::vector<vk::Image> images;
		vk::Format image_format;
		uint32_t sample_count = 1;
		std::unique_ptr<VulkanRenderTargetAttachment> color_attachment;
		std::unique_ptr<VulkanRenderTargetAttachment> depth_attachment; // TODO: remove from swapchain
	};

} }