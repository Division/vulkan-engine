#include "Engine.h"
#include "render/texture/Texture.h"

#include "VulkanRenderTarget.h"
#include "VulkanRenderPass.h"
#include "VulkanSwapchain.h"

namespace core { namespace Device {

	vk::ImageView VulkanRenderTargetAttachment::GetImageView(uint32_t frame) const 
	{ 
		return type == Type::Color ? frames[frame].image_view : depth_texture->GetImageView(); 
	}

	vk::Image VulkanRenderTargetAttachment::GetImage(uint32_t frame) const 
	{ 
		return type == Type::Color ? frames[frame].image : depth_texture->GetImage(); 
	}

	VulkanRenderTargetAttachment::VulkanRenderTargetAttachment(Type type, uint32_t width, uint32_t height, Format format, uint32_t sample_count)
		: type(type), format(format), width(width), height(height)
	{
		TextureInitializer texture_init(width, height, 1, 1, format);

		switch (type)
		{
		case Type::Color:
			texture_init.SetColorTarget().SetSampled();
			for (int i = 0; i < frames.size(); i++)
			{
				frames[i].color_texture = std::make_shared<Texture>(texture_init);
				frames[i].image_view = frames[i].color_texture->GetImageView();
				frames[i].image = frames[i].color_texture->GetImage();
			}
			break;

		case Type::Depth:
			texture_init.SetDepth().SetSampled();
			depth_texture = std::make_shared<Texture>(texture_init);
			break;
		}
	}

	VulkanRenderTargetAttachment::VulkanRenderTargetAttachment(VulkanSwapchain* swapchain)
		: swapchain(swapchain), width(swapchain->GetWidth()), height(swapchain->GetHeight()), type(Type::Color), format(swapchain->GetImageFormat())
	{
		auto device = Engine::GetVulkanDevice();
		auto& swapchain_images = swapchain->GetImages();
		for (int i = 0; i < frames.size(); i++)
		{
			vk::ImageSubresourceRange range(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
			vk::ImageViewCreateInfo view_create_info({}, swapchain_images[i], vk::ImageViewType::e2D, (vk::Format)format, vk::ComponentMapping(), range);
			frames[i].swapchain_image_view = device.createImageViewUnique(view_create_info);
			frames[i].image_view = frames[i].swapchain_image_view.get();
			frames[i].image = swapchain_images[i];
		}
	}

	VulkanRenderTargetInitializer::VulkanRenderTargetInitializer(VulkanSwapchain* swapchain)
	{
		memset(&color_attachments, 0, sizeof(color_attachments));
		this->swapchain = swapchain;
		this->width = swapchain->GetWidth();
		this->height = swapchain->GetHeight();
	}

	VulkanRenderTarget::VulkanRenderTarget(const VulkanRenderTargetInitializer& initializer)
		: sample_count(initializer.sample_count)
		, swapchain(initializer.swapchain)
		, color_attachment_count(initializer.color_attachment_count)
		, color_attachments(initializer.color_attachments)
		, depth_attachment(initializer.depth_attachment)
		, width(initializer.width)
		, height(initializer.height)
	{
		auto device = Engine::GetVulkanDevice();
		VulkanRenderPassInitializer render_pass_initializer;

		const vk::Image* images = nullptr;
		std::vector<vk::ImageView> attachments[caps::MAX_FRAMES_IN_FLIGHT];
		for (int i = 0; i < color_attachment_count; i++)
		{
			assert(color_attachments[i]->GetType() == VulkanRenderTargetAttachment::Type::Color);
			assert(color_attachments[i]->GetWidth() == width && color_attachments[i]->GetHeight() == height);
			render_pass_initializer.AddColorAttachment(color_attachments[i]->GetFormat());
			render_pass_initializer.SetLoadStoreOp(AttachmentLoadOp::DontCare, AttachmentStoreOp::Store);
			auto final_layout = color_attachments[i]->IsSwapchain() ? ImageLayout::PresentSrc : ImageLayout::ColorAttachmentOptimal;
			render_pass_initializer.SetImageLayout(ImageLayout::Undefined, final_layout);

			for (int j = 0; j < caps::MAX_FRAMES_IN_FLIGHT; j++)
				attachments[j].push_back(color_attachments[i]->GetImageView(j));
		}

		if (depth_attachment)
		{
			assert(depth_attachment->GetType() == VulkanRenderTargetAttachment::Type::Depth);
			assert(depth_attachment->GetWidth() == width && depth_attachment->GetHeight() == height);
			render_pass_initializer.AddDepthAttachment(depth_attachment->GetFormat());
			render_pass_initializer.SetLoadStoreOp(AttachmentLoadOp::DontCare, AttachmentStoreOp::Store);
			render_pass_initializer.SetImageLayout(ImageLayout::Undefined, ImageLayout::DepthStencilAttachmentOptimal);
			
			for (int j = 0; j < caps::MAX_FRAMES_IN_FLIGHT; j++)
				attachments[j].push_back(depth_attachment->GetImageView(j));
		}

		VulkanRenderPass render_pass(render_pass_initializer);

		for (int j = 0; j < caps::MAX_FRAMES_IN_FLIGHT; j++)
		{
			vk::FramebufferCreateInfo framebuffer_create_info({}, render_pass.GetRenderPass(), attachments[j].size(), attachments[j].data(), width, height, 1);
			framebuffers[j] = device.createFramebufferUnique(framebuffer_create_info);
		}
	}

} }