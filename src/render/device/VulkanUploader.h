#pragma once

#include "CommonIncludes.h"

namespace core { namespace Device {

	class VulkanBuffer;

	class VulkanUploader
	{
	public:
		struct Upload
		{
			std::shared_ptr<VulkanBuffer> src_buffer;
			VulkanBuffer* dst_buffer;
			VkDeviceSize size;
		};

		void AddToUpload(std::unique_ptr<VulkanBuffer>, VulkanBuffer* dstBuffer, VkDeviceSize size);
		void ProcessUpload();

	private:
		// These uploads are executed before frame render start (by memory barrier)
		std::vector<Upload> current_frame_uploads;
	};

} }