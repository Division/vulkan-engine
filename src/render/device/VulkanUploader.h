#pragma once

#include "CommonIncludes.h"
#include "VulkanCaps.h"

namespace core { namespace Device {

	class VulkanBuffer;
	class VulkanCommandBuffer;

	class VulkanUploader
	{
	public:
		struct Upload
		{
			Upload(std::unique_ptr<VulkanBuffer> src_buffer, VulkanBuffer* dst_buffer, VkDeviceSize size);
			Upload(Upload&& other);
			~Upload();

			std::unique_ptr<VulkanBuffer> src_buffer;
			VulkanBuffer* dst_buffer = nullptr;
			VkDeviceSize size = 0;
		};

		void AddToUpload(std::unique_ptr<VulkanBuffer>, VulkanBuffer* dstBuffer, VkDeviceSize size);
		void ProcessUpload();
		
		VulkanUploader();
		~VulkanUploader();
		
	private:
		// These uploads are executed before frame render start (by memory barrier)
		std::vector<Upload> current_frame_uploads;

		// Need to keep staging buffers alive until next frame starts (which means all uploads are done)
		std::vector<std::unique_ptr<VulkanBuffer>> buffers_in_upload[caps::MAX_FRAMES_IN_FLIGHT];
		unsigned current_frame = 0;
	};

} }