#include "VulkanRenderTarget.h"
#include "VulkanRenderPass.h"
#include "VulkanSwapchain.h"
#include "Engine.h"

namespace core { namespace Device {

	
	VulkanRenderTarget::VulkanRenderTarget(VulkanRenderTargetInitializer initializer)
		: sample_count(initializer.sample_count)
		, use_swapchain(initializer.use_swapchain)
		, swapchain(initializer.swapchain)
		, has_color(initializer.has_color)
		, has_depth(initializer.has_depth)
		, render_pass(initializer.render_pass)
		, color_format(initializer.color_format)
		, depth_format(initializer.depth_format)

	{
		Resize(initializer.width, initializer.height);
	}

	void VulkanRenderTarget::Resize(uint32_t width, uint32_t height)
	{
		auto device = Engine::GetVulkanDevice();

		const vk::Image* images;
		vk::Format image_format;
		if (use_swapchain)
		{
			width = swapchain->GetWidth();
			height = swapchain->GetHeight();
			images = swapchain->GetImages().data();
			image_format = swapchain->GetImageFormat();
			frames.resize(swapchain->GetImages().size());
		} else
		{
			throw std::runtime_error("Offscreen render targets not supported yet");
		}

		// Frame data: image views and framebuffers
		for (size_t i = 0; i < frames.size(); i++)
		{
			auto& frame = frames[i];

			vk::ImageSubresourceRange range(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
			vk::ImageViewCreateInfo view_create_info({}, images[i], vk::ImageViewType::e2D, image_format, vk::ComponentMapping(), range);
			frame.image_view = device.createImageViewUnique(view_create_info);

			vk::ImageView attachments[] = {
				frame.image_view.get()
			};

			vk::FramebufferCreateInfo framebuffer_create_info({}, render_pass->GetRenderPass(), 1, attachments, width, height, 1);
			frame.framebuffer = device.createFramebufferUnique(framebuffer_create_info);
		}
	}

} }