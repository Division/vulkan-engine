#include "Engine.h"
#include "render/texture/Texture.h"

#include "VulkanRenderTarget.h"
#include "VulkanRenderPass.h"
#include "VulkanSwapchain.h"

namespace core { namespace Device {

	vk::ImageView VulkanRenderTargetAttachment::GetImageView() const 
	{ 
		return type == Type::Color ? frame.image_view : depth_texture->GetImageView(); 
	}

	vk::Image VulkanRenderTargetAttachment::GetImage() const 
	{ 
		return type == Type::Color ? frame.image : depth_texture->GetImage(); 
	}

	VulkanRenderTargetAttachment::VulkanRenderTargetAttachment(const std::string& name, Type type, uint32_t width, uint32_t height, Format format, uint32_t sample_count)
		: type(type), format(format), width(width), height(height)
	{
		TextureInitializer texture_init(width, height, 1, 1, format);
		texture_init.name = name;

		switch (type)
		{
		case Type::Color:
			texture_init.SetColorTarget().SetSampled();
			frame.color_texture = std::make_shared<Texture>(texture_init);
			frame.image_view = frame.color_texture->GetImageView();
			frame.image = frame.color_texture->GetImage();
			break;

		case Type::Depth:
			texture_init.SetDepth().SetSampled();
			depth_texture = std::make_shared<Texture>(texture_init);
			break;
		}
	}

	VulkanRenderTargetAttachment::VulkanRenderTargetAttachment(VulkanSwapchain* swapchain, uint32_t image_index)
		: swapchain(swapchain), width(swapchain->GetWidth()), height(swapchain->GetHeight()), type(Type::Color), format(swapchain->GetImageFormat())
	{
		auto device = Engine::GetVulkanDevice();
		auto& swapchain_images = swapchain->GetImages();
		vk::ImageSubresourceRange range(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
		vk::ImageViewCreateInfo view_create_info({}, swapchain_images[image_index], vk::ImageViewType::e2D, (vk::Format)format, vk::ComponentMapping(), range);
		frame.swapchain_image_view = device.createImageViewUnique(view_create_info);
		frame.image_view = frame.swapchain_image_view.get();
		frame.image = swapchain_images[image_index];
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
		std::vector<vk::ImageView> attachments;
		for (int i = 0; i < color_attachment_count; i++)
		{
			assert(color_attachments[i]->GetType() == VulkanRenderTargetAttachment::Type::Color);
			assert(color_attachments[i]->GetWidth() == width && color_attachments[i]->GetHeight() == height);
			render_pass_initializer.AddColorAttachment(color_attachments[i]->GetFormat());
			render_pass_initializer.SetLoadStoreOp(AttachmentLoadOp::DontCare, AttachmentStoreOp::Store);
			auto final_layout = color_attachments[i]->IsSwapchain() ? ImageLayout::PresentSrc : ImageLayout::ColorAttachmentOptimal;
			render_pass_initializer.SetImageLayout(ImageLayout::Undefined, final_layout);

			attachments.push_back(color_attachments[i]->GetImageView());
		}

		if (depth_attachment)
		{
			assert(depth_attachment->GetType() == VulkanRenderTargetAttachment::Type::Depth);
			assert(depth_attachment->GetWidth() == width && depth_attachment->GetHeight() == height);
			render_pass_initializer.AddDepthAttachment(depth_attachment->GetFormat());
			render_pass_initializer.SetLoadStoreOp(AttachmentLoadOp::DontCare, AttachmentStoreOp::Store);
			render_pass_initializer.SetImageLayout(ImageLayout::Undefined, ImageLayout::DepthStencilAttachmentOptimal);
			
			attachments.push_back(depth_attachment->GetImageView());
		}

		VulkanRenderPass render_pass(render_pass_initializer);

		vk::FramebufferCreateInfo framebuffer_create_info({}, render_pass.GetRenderPass(), attachments.size(), attachments.data(), width, height, 1);
		framebuffer = device.createFramebufferUnique(framebuffer_create_info);
	}

} }