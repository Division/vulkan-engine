#pragma once

#include "CommonIncludes.h"
#include "Types.h"
#include "utils/Math.h"

namespace Device {

	class VulkanRenderTargetAttachment;

	struct VulkanRenderPassInitializer {

		VulkanRenderPassInitializer(Format format, bool has_depth, Format depth_format = Format::D24_unorm_S8_uint) 
		{
			AddColorAttachment(format);
			SetLoadStoreOp(AttachmentLoadOp::Clear, AttachmentStoreOp::Store);
			SetImageLayout(ImageLayout::Undefined, ImageLayout::PresentSrc);
			if (has_depth)
			{
				AddDepthAttachment(depth_format);
				SetLoadStoreOp(AttachmentLoadOp::Clear, AttachmentStoreOp::Store);
				SetImageLayout(ImageLayout::Undefined, ImageLayout::DepthStencilAttachmentOptimal);
			}
		}

		explicit VulkanRenderPassInitializer(vk::RenderPass renderPass)
		{
			referencedPass = renderPass;
		}

		VulkanRenderPassInitializer() = default;

		VulkanRenderPassInitializer& AddColorAttachment(Format format)
		{
			color_attachments.push_back(vk::AttachmentDescription({}, vk::Format(format)));
			current_attachment = &color_attachments.back();
			return *this;
		}

		VulkanRenderPassInitializer& AddAttachment(const VulkanRenderTargetAttachment& attachment);

		VulkanRenderPassInitializer& AddDepthAttachment(Format format)
		{
			depth_attachment = vk::AttachmentDescription({}, vk::Format(format));
			depth_attachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
			depth_attachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
			current_attachment = &depth_attachment;
			has_depth = true;
			return *this;
		}

		VulkanRenderPassInitializer& SetLoadStoreOp(AttachmentLoadOp load_op, AttachmentStoreOp store_op)
		{
			current_attachment->setLoadOp(vk::AttachmentLoadOp(load_op));
			current_attachment->setStoreOp(vk::AttachmentStoreOp(store_op));
			return *this;
		}

		VulkanRenderPassInitializer& SetImageLayout(ImageLayout src_layout, ImageLayout dst_layout)
		{
			current_attachment->setInitialLayout(vk::ImageLayout(src_layout));
			current_attachment->setFinalLayout(vk::ImageLayout(dst_layout));
			return *this;
		}

		uint32_t GetHash() const
		{
			if (referencedPass)
			{
				return FastHash(&referencedPass, sizeof(referencedPass));
			}

			uint32_t color_attachments_hash = 0;
			uint32_t depth_attachment_hash = 0;
			if (has_depth)
				depth_attachment_hash = FastHash(&depth_attachment, sizeof(vk::AttachmentDescription));

			if (color_attachments.size())
				color_attachments_hash = FastHash(color_attachments.data(), sizeof(vk::AttachmentDescription) * color_attachments.size());

			std::array<uint32_t, 2> hashes = {
				depth_attachment_hash, color_attachments_hash
			};
			
			return FastHash(hashes.data(), sizeof(hashes));
		}

		vk::AttachmentDescription* current_attachment = nullptr;
		std::vector<vk::AttachmentDescription> color_attachments;
		vk::AttachmentDescription depth_attachment;
		vk::RenderPass referencedPass;
		bool has_depth = false;
	};

	class VulkanRenderPass
	{
	public:
		VulkanRenderPass(const VulkanRenderPassInitializer& initializer);

		vk::RenderPass GetRenderPass() const { return isReference ? referencedPass : render_pass.get(); }

		bool HasDepth() const { assert(!isReference); return has_depth; }
		Format GetDepthFormat() const { assert(!isReference); return (Format)depth_format; }

		uint32_t GetHash() const { return hash; };

	private:
		static std::atomic_int counter;
		bool isReference = false;
		uint32_t hash;
		vk::UniqueRenderPass render_pass;
		vk::RenderPass referencedPass;
		bool has_depth;
		vk::Format depth_format = vk::Format::eD24UnormS8Uint;
	};

}