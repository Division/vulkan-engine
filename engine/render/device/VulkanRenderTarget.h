#pragma once

#include "CommonIncludes.h"
#include "VulkanCaps.h"
#include "Types.h"

#include "utils/Math.h"

namespace Device {

	class VulkanRenderPass;
	class VulkanSwapchain;
	class Texture;

	class VulkanRenderTargetAttachment
	{
	public:
		enum class Type : int
		{
			Color,
			Depth
		};

		struct Frame
		{
			vk::Image image;
			vk::ImageView image_view;
			vk::UniqueImageView swapchain_image_view;
			std::shared_ptr<Texture> color_texture;
		};

		vk::ImageView GetImageView() const;
		vk::Image GetImage() const;
		std::shared_ptr<Texture>& GetTexture() { return type == Type::Depth ? depth_texture : frame.color_texture; }
		VulkanRenderTargetAttachment(const std::string& name, Type type, uint32_t width, uint32_t height, Format format, uint32_t sample_count = 1);
		VulkanRenderTargetAttachment(VulkanSwapchain* swapchain, uint32_t image_index);
		bool IsSwapchain() const { return (bool)swapchain; }

		Format GetFormat() const { return format; }
		Type GetType() const { return type; }
		uint32_t GetWidth() const { return width; }
		uint32_t GetHeight() const { return height; }

	private:
		Type type;
		Format format;
		uint32_t width;
		uint32_t height;

		VulkanSwapchain* swapchain = nullptr;
		std::shared_ptr<Texture> depth_texture;
		Frame frame;
	};

	struct VulkanRenderTargetInitializer {

		VulkanRenderTargetInitializer(uint32_t width, uint32_t height)
		{
			memset(&color_attachments, 0, sizeof(color_attachments));
			Size(width, height);
		}

		VulkanRenderTargetInitializer(VulkanSwapchain* swapchain);

		VulkanRenderTargetInitializer(VulkanRenderTargetInitializer&&) = default;
		VulkanRenderTargetInitializer& operator=(VulkanRenderTargetInitializer&&) = default;

		VulkanRenderTargetInitializer& Size(uint32_t width, uint32_t height) 
		{
			this->width = width;
			this->height = height;
			return *this;
		}

		VulkanRenderTargetInitializer& AddAttachment(VulkanRenderTargetAttachment& attachment) 
		{
			if (attachment.GetType() == VulkanRenderTargetAttachment::Type::Color)
			{
				has_color = true;
				color_attachments[color_attachment_count] = &attachment;
				color_attachment_count += 1;
			} else
			{
				assert(!depth_attachment && depth_attachment != &attachment);
				has_depth = true;
				depth_attachment = &attachment;
			}
			
			return *this;
		}
	
		uint32_t GetHash() const
		{
			auto depth = (size_t)depth_attachment;
			uint32_t hashes[] = { FastHash(color_attachments.data(), sizeof(color_attachments)), FastHash(&depth, sizeof(depth)) };
			return FastHash(hashes, sizeof(hashes));
		}

		uint32_t sample_count = 1;
		uint32_t width = 0;
		uint32_t height = 0;
		bool use_swapchain = false;
		bool has_color = false;
		bool has_depth = false;
		VulkanSwapchain* swapchain = nullptr;
		std::array<VulkanRenderTargetAttachment*, caps::max_color_attachments> color_attachments;
		uint32_t color_attachment_count = 0;
		VulkanRenderTargetAttachment* depth_attachment = nullptr;
	};

	class VulkanRenderTarget
	{
	public:
		VulkanRenderTarget(const VulkanRenderTargetInitializer& initializer);

		uint32_t GetWidth() const { return width; }
		uint32_t GetHeight() const { return height; }
		vk::Framebuffer GetFramebuffer() const { return framebuffer.get(); }
		bool IsSwapchain() const { return swapchain != nullptr; }
		bool HasColor() const { return color_attachment_count > 0; }
		bool HasDepth() const { return (bool)depth_attachment; }
		uint32_t GetColorAttachmentCount() const { return color_attachment_count; }
		uint32_t GetOwnershipQueue() const { return ownership_queue; }
		uint32_t SetOwnershipQueue(uint32_t value) { ownership_queue = value; }

	private:
		uint32_t color_attachment_count = 0;
		std::array<VulkanRenderTargetAttachment*, caps::max_color_attachments> color_attachments;
		VulkanRenderTargetAttachment* depth_attachment;
		vk::UniqueFramebuffer framebuffer;

		uint32_t ownership_queue = -1;
		uint32_t sample_count = 1;
		uint32_t width = 0;
		uint32_t height = 0;
		VulkanSwapchain* swapchain = nullptr;
	};

}