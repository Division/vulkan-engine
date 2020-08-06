#include <mutex>
#include "render/buffer/VulkanBuffer.h"
#include "Engine.h"
#include "CommonIncludes.h"
#include "VulkanUploader.h"
#include "VulkanContext.h"
#include "VkObjects.h"

namespace core { namespace Device {

	std::mutex uploader_mutex;

	VulkanUploader::UploadBase::UploadBase() = default;
	VulkanUploader::UploadBase::~UploadBase() = default;
	VulkanUploader::UploadBase::UploadBase(UploadBase&&) = default;

	VulkanUploader::BufferUpload::BufferUpload(VulkanBuffer* src_buffer, VulkanBuffer* dst_buffer, VkDeviceSize size) : UploadBase()
		, dst_buffer(dst_buffer)
		, size(size)
	{
		this->src_buffer = std::move(src_buffer);
	}
	
	VulkanUploader::BufferUpload::BufferUpload(BufferUpload&& other) = default;
	VulkanUploader::BufferUpload::~BufferUpload() = default;
	
	void VulkanUploader::BufferUpload::Process(vk::CommandBuffer& command_buffer)
	{
		VkBufferCopy copy_region = { 0, 0, size };
		vkCmdCopyBuffer(command_buffer, src_buffer->Buffer(), dst_buffer->Buffer(), 1, &copy_region);
		
		VkMemoryBarrier memoryBarrier = {
			VK_STRUCTURE_TYPE_MEMORY_BARRIER,
			nullptr,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT
		};

		vkCmdPipelineBarrier(
			command_buffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,      // srcStageMask
			VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,   // dstStageMask
			0,
			1,                                   // memoryBarrierCount
			&memoryBarrier,                       // pMemoryBarriers
			0, nullptr,
			0, nullptr
		);
	}

	void TransitionImageLayout(vk::CommandBuffer& command_buffer, vk::Image& image, vk::ImageLayout old_layout, vk::ImageLayout new_layout, uint32_t mip_level_count, uint32_t layer_count)
	{
		vk::ImageMemoryBarrier barrier;
		barrier.oldLayout = old_layout;
		barrier.newLayout = new_layout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = mip_level_count;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = layer_count;

		vk::PipelineStageFlags source_stage;
		vk::PipelineStageFlags destination_stage;

		if (old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eTransferDstOptimal) {
			barrier.srcAccessMask = vk::AccessFlags();
			barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

			source_stage = vk::PipelineStageFlagBits::eTopOfPipe;
			destination_stage = vk::PipelineStageFlagBits::eTransfer;
		}
		else if (old_layout == vk::ImageLayout::eTransferDstOptimal && new_layout == vk::ImageLayout::eShaderReadOnlyOptimal) {
			barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

			source_stage = vk::PipelineStageFlagBits::eTransfer;
			destination_stage = vk::PipelineStageFlagBits::eFragmentShader;
		}
		else {
			throw std::invalid_argument("unsupported layout transition!");
		}

		command_buffer.pipelineBarrier(source_stage, destination_stage, vk::DependencyFlags(), 0, nullptr, 0, nullptr, 1, &barrier);
	}
	
	VulkanUploader::ImageUpload::ImageUpload(VulkanBuffer* src_buffer, vk::Image dst_image, uint32_t mip_count, uint32_t array_count, std::vector<vk::BufferImageCopy> copies)
		: dst_image(dst_image)
		, mip_count(mip_count)
		, array_count(array_count)
		, copies(std::move(copies))
	{
		this->src_buffer = std::move(src_buffer);
	}

	VulkanUploader::ImageUpload::ImageUpload(ImageUpload&& other) = default;
	VulkanUploader::ImageUpload::~ImageUpload() = default;

	void VulkanUploader::ImageUpload::Process(vk::CommandBuffer& command_buffer)
	{
		TransitionImageLayout(command_buffer, dst_image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, mip_count, array_count);
		command_buffer.copyBufferToImage(src_buffer->Buffer(), dst_image, vk::ImageLayout::eTransferDstOptimal, copies.size(), copies.data());
		TransitionImageLayout(command_buffer, dst_image, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, mip_count, array_count);
	}

	VulkanUploader::VulkanUploader() 
	{
		auto* context = Engine::Get()->GetContext();
		command_pool = std::make_unique<VulkanCommandPool>(context->GetQueueFamilyIndex(PipelineBindPoint::Graphics));
		for (int i = 0; i < caps::MAX_FRAMES_IN_FLIGHT; i++)
			command_buffers[i] = command_pool->GetCommandBuffer();
	};

	VulkanUploader::~VulkanUploader() {};

	void VulkanUploader::AddToUpload(VulkanBuffer* src_buffer, VulkanBuffer* dst_buffer, vk::DeviceSize size)
	{
		std::lock_guard<std::mutex> lock(uploader_mutex);
		current_frame_uploads.emplace_back(std::make_unique<BufferUpload>(src_buffer, dst_buffer, size));
	}

	void VulkanUploader::AddImageToUpload(VulkanBuffer* src_buffer, vk::Image dst_image, uint32_t mip_count, uint32_t array_count, std::vector<vk::BufferImageCopy> copies)
	{
		std::lock_guard<std::mutex> lock(uploader_mutex);
		current_frame_uploads.emplace_back(std::make_unique<ImageUpload>(std::move(src_buffer), dst_image, mip_count, array_count, std::move(copies)));
	}

	void VulkanUploader::ProcessUpload()
	{
		std::vector<std::unique_ptr<UploadBase>> uploads_copy;
		{
			std::lock_guard<std::mutex> lock(uploader_mutex);
			if (!current_frame_uploads.size())
				return;

			uploads_copy = std::move(current_frame_uploads);
		}

		auto* context = Engine::GetVulkanContext();
		auto command_buffer = command_buffers[current_frame]->GetCommandBuffer();

		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(command_buffer, &begin_info);

		for (auto& upload : uploads_copy)
			upload->Process(command_buffer);

		vkEndCommandBuffer(command_buffer);
		
		auto graphicsQueue = context->GetGraphicsQueue();
		vk::SubmitInfo submitInfo(0, nullptr, nullptr, 1, &command_buffer);
		((vk::Queue)graphicsQueue).submit(1u, &submitInfo, vk::Fence());

		current_frame = (current_frame + 1) % caps::MAX_FRAMES_IN_FLIGHT;
	}

}


}