#include "Engine.h"
#include "render/texture/Texture.h"

#include "VulkanRenderTarget.h"
#include "VulkanRenderPass.h"
#include "VulkanSwapchain.h"

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

		const vk::Image* images = nullptr;
		vk::Format image_format;
		if (use_swapchain)
		{
			has_color = true;
			width = swapchain->GetWidth();
			height = swapchain->GetHeight();
			images = swapchain->GetImages().data();
			image_format = swapchain->GetImageFormat();
			frames.resize(swapchain->GetImages().size());
		} else if (has_color)
		{
			image_format = color_format;
			TextureInitializer color_texture_init(width, height, 1, 1, (Format)color_format);
			color_texture_init.SetColorTarget();
			for (int i = 0; i < color_textures.size(); i++)
			{
				color_textures[i] = std::make_shared<Texture>(color_texture_init);
				color_texture_images[i] = color_textures[i]->GetImage();
			}
			images = color_texture_images.data();
			frames.resize(color_textures.size());
		}

		has_depth = render_pass->HasDepth();

		if (has_depth)
		{
			TextureInitializer depth_texture_init(width, height, 1, 1, (Format)render_pass->GetDepthFormat());
			depth_texture_init.SetDepth();
			depth_texture = std::make_shared<Texture>(depth_texture_init);
		}

		// Frame data: image views and framebuffers
		if (has_color)
		{
			for (size_t i = 0; i < frames.size(); i++)
			{
				auto& frame = frames[i];

				vk::ImageSubresourceRange range(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
				vk::ImageViewCreateInfo view_create_info({}, images[i], vk::ImageViewType::e2D, image_format, vk::ComponentMapping(), range);
				frame.image_view = device.createImageViewUnique(view_create_info);

				vk::ImageView attachments[] = {
					frame.image_view.get(),
					has_depth ? depth_texture->GetImageView() : vk::ImageView()
				};

				uint32_t attachment_count = has_depth ? 2 : 1;

				vk::FramebufferCreateInfo framebuffer_create_info({}, render_pass->GetRenderPass(), attachment_count, attachments, width, height, 1);
				frame.framebuffer = device.createFramebufferUnique(framebuffer_create_info);
			}
		}

		this->width = width;
		this->height = height;
	}

} }