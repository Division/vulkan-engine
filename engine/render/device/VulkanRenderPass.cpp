#include "VulkanRenderPass.h"
#include "Engine.h"
#include "VulkanContext.h"

namespace core { namespace Device {

	VulkanRenderPass::VulkanRenderPass(const VulkanRenderPassInitializer& initializer)
	{
		has_depth = initializer.has_depth;
		hash = initializer.GetHash();

		std::vector<vk::AttachmentDescription> attachments;
		std::vector<vk::AttachmentReference> attachment_references;

		for (int i = 0; i < initializer.color_attachments.size(); i++)
		{
			attachments.push_back(initializer.color_attachments[i]);
			attachment_references.emplace_back(attachments.size() - 1, vk::ImageLayout::eColorAttachmentOptimal);
		}

		vk::AttachmentReference* depth_attachment_ref = nullptr;
		if (has_depth)
		{
			attachments.push_back(initializer.depth_attachment);
			attachment_references.emplace_back(attachments.size() - 1, vk::ImageLayout::eDepthStencilAttachmentOptimal);
			depth_attachment_ref = &attachment_references.back();
		}

		vk::SubpassDescription subpass_desc(
			{},
			vk::PipelineBindPoint::eGraphics,
			0,
			nullptr,
			initializer.color_attachments.size(),
			attachment_references.data(),
			nullptr,
			depth_attachment_ref
		);

		// TODO: remove subpass dependency
		vk::SubpassDependency subpass_dependency(
			VK_SUBPASS_EXTERNAL,
			0,
			vk
			::PipelineStageFlagBits::eColorAttachmentOutput, 
			vk::PipelineStageFlagBits::eColorAttachmentOutput, 
			{},
			vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite
		);

		vk::RenderPassCreateInfo render_pass_info({}, attachments.size(), attachments.data(), 1, &subpass_desc, 1, &subpass_dependency);
		
		render_pass = Engine::GetVulkanContext()->GetDevice().createRenderPassUnique(render_pass_info);
	}

	std::atomic_int VulkanRenderPass::counter = 0;

} }