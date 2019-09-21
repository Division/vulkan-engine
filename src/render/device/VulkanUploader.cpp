#include <mutex>
#include "render/buffer/VulkanBuffer.h"
#include "Engine.h"
#include "CommonIncludes.h"
#include "VulkanUploader.h"
#include "VulkanContext.h"
#include "VkObjects.h"
#include "CommandBufferManager.h"

namespace core { namespace Device {

	std::mutex uploader_mutex;

	VulkanUploader::Upload::Upload(std::unique_ptr<VulkanBuffer> src_buffer, VulkanBuffer* dst_buffer, VkDeviceSize size)
		: src_buffer(std::move(src_buffer))
		, dst_buffer(dst_buffer)
		, size(size)
	{}
	
	VulkanUploader::Upload::Upload(Upload&& other) = default;
	VulkanUploader::Upload::~Upload() = default;
	VulkanUploader::VulkanUploader() {};
	VulkanUploader::~VulkanUploader() {};

	void VulkanUploader::AddToUpload(std::unique_ptr<VulkanBuffer> src_buffer, VulkanBuffer* dst_buffer, VkDeviceSize size)
	{
		std::lock_guard<std::mutex> guard(uploader_mutex);
		current_frame_uploads.emplace_back(std::move(src_buffer), dst_buffer, size);
	}

	void VulkanUploader::ProcessUpload()
	{
		buffers_in_upload[current_frame].clear();

		if (!current_frame_uploads.size())
			return;

		std::vector<Upload> uploads_copy;
		{
			std::lock_guard<std::mutex> guard(uploader_mutex);
			uploads_copy = std::move(current_frame_uploads);
		}

		auto* context = Engine::GetVulkanContext();
		// TODO: check if it's better to use different pool
		auto* command_buffer = context->GetCommandBufferManager()->GetDefaultCommandPool()->GetCommandBuffer();

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(command_buffer->GetCommandBuffer(), &beginInfo);

		for (int i = 0; i < uploads_copy.size(); i++)
		{
			auto srcBuffer = uploads_copy[i].src_buffer->Buffer();
			auto dstBuffer = uploads_copy[i].dst_buffer->Buffer();
			VkBufferCopy copyRegion = { 0, 0, uploads_copy[i].size };
			vkCmdCopyBuffer(command_buffer->GetCommandBuffer(), srcBuffer, dstBuffer, 1, &copyRegion);
			buffers_in_upload[current_frame].push_back(std::move(uploads_copy[i].src_buffer));
		}

		VkMemoryBarrier memoryBarrier = {
			VK_STRUCTURE_TYPE_MEMORY_BARRIER, 
			nullptr,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT
		};

		auto vk_command_buffer = command_buffer->GetCommandBuffer();

		vkCmdPipelineBarrier(
			vk_command_buffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,      // srcStageMask
			VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,   // dstStageMask
			0,
			1,                                   // memoryBarrierCount
			&memoryBarrier,                       // pMemoryBarriers
			0, nullptr,
			0, nullptr
		);

		vkEndCommandBuffer(vk_command_buffer);

		auto graphicsQueue = context->GetGraphicsQueue();
		vk::SubmitInfo submitInfo(0, nullptr, nullptr, 1, &vk_command_buffer);
		((vk::Queue)graphicsQueue).submit(1u, &submitInfo, vk::Fence());

		current_frame = (current_frame + 1) % caps::MAX_FRAMES_IN_FLIGHT;
	}

}


}