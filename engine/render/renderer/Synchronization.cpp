#include "Synchronization.h"
#include "Engine.h"
#include "render/device/VulkanContext.h"

namespace core::render::synchronization {

	ImageAccess GetImageAccessSrc(ResourceOperationType operation_type)
	{
		ImageAccess result;

		auto* context = Engine::Get()->GetContext();
		auto compute_index = context->GetQueueFamilyIndex(core::Device::PipelineBindPoint::Compute);
		auto graphics_index = context->GetQueueFamilyIndex(core::Device::PipelineBindPoint::Graphics);

		switch (operation_type)
		{
			case ResourceOperationType::GraphicsShaderRead:
				result.access = vk::AccessFlags();
				result.layout = vk::ImageLayout::eShaderReadOnlyOptimal;
				result.stage = vk::PipelineStageFlagBits::eVertexShader;
				result.queue_index = graphics_index;
			break;

			case ResourceOperationType::GraphicsShaderWrite:
				result.access = vk::AccessFlagBits::eShaderWrite;
				result.layout = vk::ImageLayout::eGeneral;
				result.stage = vk::PipelineStageFlagBits::eVertexShader | vk::PipelineStageFlagBits::eFragmentShader;
				result.queue_index = graphics_index;
			break;

			case ResourceOperationType::GraphicsShaderReadWrite:
				result.access = vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead;
				result.layout = vk::ImageLayout::eGeneral;
				result.stage = vk::PipelineStageFlagBits::eVertexShader | vk::PipelineStageFlagBits::eFragmentShader;
				result.queue_index = graphics_index;
			break;

			case ResourceOperationType::ComputeShaderRead:
				result.access = vk::AccessFlags();
				result.layout = vk::ImageLayout::eShaderReadOnlyOptimal;
				result.stage = vk::PipelineStageFlagBits::eComputeShader;
				result.queue_index = compute_index;
			break;

			case ResourceOperationType::ComputeShaderWrite:
				result.access = vk::AccessFlagBits::eShaderWrite;
				result.layout = vk::ImageLayout::eGeneral;
				result.stage = vk::PipelineStageFlagBits::eComputeShader;
				result.queue_index = compute_index;
			break;

			case ResourceOperationType::ComputeShaderReadWrite:
				result.access = vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead;
				result.layout = vk::ImageLayout::eGeneral;
				result.stage = vk::PipelineStageFlagBits::eComputeShader;
				result.queue_index = compute_index;
			break;

			case ResourceOperationType::ColorAttachment:
				result.access = vk::AccessFlagBits::eColorAttachmentWrite;
				result.layout = vk::ImageLayout::eColorAttachmentOptimal;
				result.stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
				result.queue_index = graphics_index;
			break;

			case ResourceOperationType::DepthStencilAttachment:
				result.access = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
				result.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
				result.stage = vk::PipelineStageFlagBits::eLateFragmentTests;
				result.queue_index = graphics_index;
			break;

			case ResourceOperationType::Present:
				result.access = vk::AccessFlagBits::eColorAttachmentRead;
				result.layout = vk::ImageLayout::ePresentSrcKHR;
				result.stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
				result.queue_index = graphics_index;
			break;
		}

		return result;
	}

	ImageAccess GetImageAccessDst(ResourceOperationType operation_type)
	{
		ImageAccess result;

		auto* context = Engine::Get()->GetContext();
		auto compute_index = context->GetQueueFamilyIndex(core::Device::PipelineBindPoint::Compute);
		auto graphics_index = context->GetQueueFamilyIndex(core::Device::PipelineBindPoint::Graphics);

		switch (operation_type)
		{
		case ResourceOperationType::GraphicsShaderRead:
			result.access = vk::AccessFlagBits::eShaderRead;
			result.layout = vk::ImageLayout::eShaderReadOnlyOptimal;
			result.stage = vk::PipelineStageFlagBits::eVertexShader;
			result.queue_index = graphics_index;
			break;

		case ResourceOperationType::GraphicsShaderWrite:
			result.access = vk::AccessFlagBits::eShaderWrite;
			result.layout = vk::ImageLayout::eGeneral;
			result.stage = vk::PipelineStageFlagBits::eVertexShader | vk::PipelineStageFlagBits::eFragmentShader;
			result.queue_index = graphics_index;
			break;

		case ResourceOperationType::GraphicsShaderReadWrite:
			result.access = vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead;
			result.layout = vk::ImageLayout::eGeneral;
			result.stage = vk::PipelineStageFlagBits::eVertexShader | vk::PipelineStageFlagBits::eFragmentShader;
			result.queue_index = graphics_index;
			break;

		case ResourceOperationType::ComputeShaderRead:
			result.access = vk::AccessFlagBits::eShaderRead;
			result.layout = vk::ImageLayout::eShaderReadOnlyOptimal;
			result.stage = vk::PipelineStageFlagBits::eComputeShader;
			result.queue_index = compute_index;
			break;

		case ResourceOperationType::ComputeShaderWrite:
			result.access = vk::AccessFlagBits::eShaderWrite;
			result.layout = vk::ImageLayout::eGeneral;
			result.stage = vk::PipelineStageFlagBits::eComputeShader;
			result.queue_index = compute_index;
			break;

		case ResourceOperationType::ComputeShaderReadWrite:
			result.access = vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead;
			result.layout = vk::ImageLayout::eGeneral;
			result.stage = vk::PipelineStageFlagBits::eComputeShader;
			result.queue_index = compute_index;
			break;

		case ResourceOperationType::ColorAttachment:
			result.access = vk::AccessFlagBits::eColorAttachmentWrite;
			result.layout = vk::ImageLayout::eColorAttachmentOptimal;
			result.stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
			result.queue_index = graphics_index;
			break;

		case ResourceOperationType::DepthStencilAttachment:
			result.access = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
			result.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
			result.stage = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
			result.queue_index = graphics_index;
			break;

		case ResourceOperationType::Present:
			result.access = vk::AccessFlagBits::eColorAttachmentRead;
			result.layout = vk::ImageLayout::ePresentSrcKHR;
			result.stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
			result.queue_index = graphics_index;
			break;
		}

		return result;
	}

	BufferAccess GetBufferAccessSrc(ResourceOperationType operation_type)
	{
		auto image_access = GetImageAccessSrc(operation_type);
		return { image_access.access, image_access.stage, image_access.queue_index };
	}

	BufferAccess GetBufferAccessDst(ResourceOperationType operation_type)
	{
		auto image_access = GetImageAccessDst(operation_type);
		return { image_access.access, image_access.stage, image_access.queue_index };
	}


	void ImagePipelineBarrier(
		const ImageAccess src_access, const ImageAccess dst_access, vk::Image image, /*range,*/ vk::CommandBuffer& command_buffer
	) {
		auto* context = Engine::GetVulkanContext();

		auto image_aspect = dst_access.layout == vk::ImageLayout::eDepthStencilAttachmentOptimal || src_access.layout == vk::ImageLayout::eDepthStencilAttachmentOptimal 
			? vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil 
			: vk::ImageAspectFlagBits::eColor;

		vk::ImageSubresourceRange range( // TODO: proper range handling, support for rendering to mip/cubemap face
			image_aspect,
			0, 1, 0, 1
		);

		uint32_t src_queue_family = VK_QUEUE_FAMILY_IGNORED;
		uint32_t dst_queue_family = VK_QUEUE_FAMILY_IGNORED;

		if (src_access.queue_index != dst_access.queue_index)
		{
			src_queue_family = src_access.queue_index;
			dst_queue_family = dst_access.queue_index;
		}

		vk::ImageMemoryBarrier image_memory_barrier(
			src_access.access, dst_access.access,
			src_access.layout, dst_access.layout,
			src_queue_family, dst_queue_family,
			image, range
		);

		command_buffer.pipelineBarrier(
			src_access.stage, 
			dst_access.stage,
			{},
			0, nullptr, 
			0, nullptr, 
			1, &image_memory_barrier
		);
	}

	void BufferPipelineBarrier(
		const BufferAccess src_access, const BufferAccess dst_access, vk::Buffer buffer, vk::DeviceSize buffer_size, vk::CommandBuffer& command_buffer
	) {
		auto* context = Engine::GetVulkanContext();

		uint32_t src_queue_family = VK_QUEUE_FAMILY_IGNORED;
		uint32_t dst_queue_family = VK_QUEUE_FAMILY_IGNORED;

		if (src_access.queue_index != dst_access.queue_index)
		{
			src_queue_family = src_access.queue_index;
			dst_queue_family = dst_access.queue_index;
		}

		vk::BufferMemoryBarrier buffer_memory_barrier(src_access.access, dst_access.access, src_queue_family, dst_queue_family, buffer, 0, buffer_size);

		command_buffer.pipelineBarrier(
			src_access.stage, 
			dst_access.stage,
			{},
			0, nullptr, 
			1, &buffer_memory_barrier,
			0, nullptr
		);
	}

}