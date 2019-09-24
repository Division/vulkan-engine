#pragma once

#include "CommonIncludes.h"
#include "VulkanCaps.h"

namespace core { namespace Device {

	class VulkanBuffer;
	class VulkanCommandBuffer;
	class VulkanCommandPool;

	class VulkanUploader
	{
	public:
		struct UploadBase
		{
			UploadBase();
			virtual ~UploadBase();
			UploadBase(UploadBase&&);

			virtual void Process(vk::CommandBuffer& command_buffer, std::vector<std::unique_ptr<VulkanBuffer>>& buffers_in_upload) {};
			std::unique_ptr<VulkanBuffer> src_buffer;
		};

		struct BufferUpload : public UploadBase
		{
			BufferUpload(std::unique_ptr<VulkanBuffer> src_buffer, VulkanBuffer* dst_buffer, VkDeviceSize size);
			BufferUpload(BufferUpload&& other);
			~BufferUpload();

			void Process(vk::CommandBuffer& command_buffer, std::vector<std::unique_ptr<VulkanBuffer>>& buffers_in_upload) override;

			VkDeviceSize size = 0;
			VulkanBuffer* dst_buffer = nullptr;
		};

		struct ImageUpload : public UploadBase
		{
			ImageUpload(std::unique_ptr<VulkanBuffer> src_buffer, vk::Image dst_image, uint32_t mip_count, uint32_t array_count, std::vector<vk::BufferImageCopy> copies);
			ImageUpload(ImageUpload&& other);
			~ImageUpload();

			void Process(vk::CommandBuffer& command_buffer, std::vector<std::unique_ptr<VulkanBuffer>>& buffers_in_upload) override;

			std::unique_ptr<VulkanBuffer> src_buffer;
			vk::Image dst_image;
			uint32_t mip_count;
			uint32_t array_count;
			std::vector<vk::BufferImageCopy> copies;
		};

		void AddToUpload(std::unique_ptr<VulkanBuffer>, VulkanBuffer* dstBuffer, vk::DeviceSize size);
		void AddImageToUpload(std::unique_ptr<VulkanBuffer> src_buffer, vk::Image dst_image, uint32_t mip_count, uint32_t array_count, std::vector<vk::BufferImageCopy> copies);
		void ProcessUpload();
		
		VulkanUploader();
		~VulkanUploader();
		
	private:
		// These uploads are executed before frame render start (by memory barrier)
		std::vector<std::unique_ptr<UploadBase>> current_frame_uploads;

		std::unique_ptr<VulkanCommandPool> command_pool;
		VulkanCommandBuffer* command_buffers[caps::MAX_FRAMES_IN_FLIGHT];
		// Need to keep staging buffers alive until next frame starts (which means all uploads are done)
		std::vector<std::unique_ptr<VulkanBuffer>> buffers_in_upload[caps::MAX_FRAMES_IN_FLIGHT];
		unsigned current_frame = 0;
	};

} }