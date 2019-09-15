#include "VulkanRenderPass.h"
#include "Engine.h"
#include "VulkanContext.h"

namespace core { namespace Device {

	VulkanRenderPass::VulkanRenderPass(const VulkanRenderPassInitializer& initializer)
	{
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

		vk::SubpassDescription subpass_desc(
			{},
			vk::PipelineBindPoint::eGraphics,
			0,
			nullptr,
			1,
			&color_attachment_ref
		);

		vk::SubpassDependency subpass_dependency(
			VK_SUBPASS_EXTERNAL,
			0, 
			vk::PipelineStageFlagBits::eColorAttachmentOutput, 
			vk::PipelineStageFlagBits::eColorAttachmentOutput, 
			{},
			vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite
		);

		vk::RenderPassCreateInfo render_pass_info({}, 1, &color_attachment, 1, &subpass_desc, 1, &subpass_dependency);
		
		render_pass = Engine::GetVulkanContext()->GetDevice().createRenderPassUnique(render_pass_info);
	}



} }