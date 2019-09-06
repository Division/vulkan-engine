#include <mutex>
#include "Engine.h"
#include "CommonIncludes.h"
#include "VulkanUploader.h"
#include "VulkanContext.h"
#include "VkObjects.h"
#include "render/buffer/VulkanBuffer.h"
#include "CommandBufferManager.h"

namespace core { namespace Device {

	std::mutex uploader_mutex;

	void VulkanUploader::AddToUpload(std::unique_ptr<VulkanBuffer> src_buffer, VulkanBuffer* dst_buffer, VkDeviceSize size)
	{
		std::lock_guard<std::mutex> guard(uploader_mutex);
		current_frame_uploads.emplace_back(Upload{ std::move(src_buffer), dst_buffer, size });
	}

	void VulkanUploader::ProcessUpload()
	{
		if (!current_frame_uploads.size())
			return;

		std::vector<Upload> uploads_copy;
		{
			std::lock_guard<std::mutex> guard(uploader_mutex);
			uploads_copy = std::move(current_frame_uploads);
		}

		auto* context = Engine::GetVulkanContext();
		auto* command_buffer = context->GetCommandBufferManager()->GetDefaultCommandPool()->GetCommandBuffer();

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(command_buffer->GetCommandBuffer(), &beginInfo);

		for (int i = 0; i < current_frame_uploads.size(); i++)
		{
			auto srcBuffer = current_frame_uploads[i].src_buffer->Buffer();
			auto dstBuffer = current_frame_uploads[i].dst_buffer->Buffer();
			VkBufferCopy copyRegion = { 0, 0, current_frame_uploads.size() };
			vkCmdCopyBuffer(command_buffer->GetCommandBuffer(), srcBuffer, dstBuffer, 1, &copyRegion);
		}

		auto vk_command_buffer = command_buffer->GetCommandBuffer();
		vkEndCommandBuffer(vk_command_buffer);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &vk_command_buffer;

		VkMemoryBarrier memoryBarrier = {
			VK_STRUCTURE_TYPE_MEMORY_BARRIER, 
			nullptr,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT 
		};

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

		auto graphicsQueue = context->GetGraphicsQueue();
		vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

		command_buffer->Release();
	}

} }