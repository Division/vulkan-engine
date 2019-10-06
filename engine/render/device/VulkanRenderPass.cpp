#include "VulkanRenderPass.h"
#include "Engine.h"
#include "VulkanContext.h"

namespace core { namespace Device {

	VulkanRenderPass::VulkanRenderPass(const VulkanRenderPassInitializer& initializer)
	{
		has_depth = initializer.has_depth;
		hash = counter.fetch_add(1);

		vk::AttachmentDescription color_attachment(
			{},
			initializer.format,
			vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::ePresentSrcKHR
		);
		vk::AttachmentReference color_attachment_ref(0, vk::ImageLayout::eColorAttachmentOptimal);
		
		vk::AttachmentDescription depth_attachment(
			{},
			depth_format,
			vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eDepthStencilAttachmentOptimal
		);

		vk::AttachmentReference depth_attachment_ref(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);

		vk::SubpassDescription subpass_desc(
			{},
			vk::PipelineBindPoint::eGraphics,
			0,
			nullptr,
			1,
			&color_attachment_ref,
			nullptr,
			has_depth ? &depth_attachment_ref : nullptr
		);

		vk::AttachmentDescription attachments[] = { color_attachment, depth_attachment };
		uint32_t attachment_count = has_depth ? 2 : 1;

		vk::SubpassDependency subpass_dependency(
			VK_SUBPASS_EXTERNAL,
			0, 
			vk::PipelineStageFlagBits::eColorAttachmentOutput, 
			vk::PipelineStageFlagBits::eColorAttachmentOutput, 
			{},
			vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite
		);

		vk::RenderPassCreateInfo render_pass_info({}, attachment_count, attachments, 1, &subpass_desc, 1, &subpass_dependency);
		
		render_pass = Engine::GetVulkanContext()->GetDevice().createRenderPassUnique(render_pass_info);
	}

	std::atomic_int VulkanRenderPass::counter = 0;

} }