#pragma once

#include "CommonIncludes.h"
#include "VulkanCaps.h"

namespace Device {
	
	class VulkanCommandPool;

	class VulkanCommandBuffer : NonCopyable
	{
	public:
		explicit VulkanCommandBuffer(VulkanCommandPool* command_pool);
		explicit VulkanCommandBuffer(VulkanCommandBuffer&& other);
		~VulkanCommandBuffer();

		vk::CommandBuffer GetCommandBuffer() { return command_buffer; }
		VulkanCommandPool* GetCommandPool() { return command_pool; }

	private:
		vk::CommandBuffer command_buffer;
		VulkanCommandPool* command_pool = nullptr;
	};


	class VulkanCommandPool : NonCopyable
	{
	public:
		VulkanCommandPool(uint32_t queue_family);
		virtual ~VulkanCommandPool();

		vk::CommandPool GetCommandPool() const { return command_pool.get(); }
		VulkanCommandBuffer* GetCommandBuffer();
		void NextFrame();

	private:
		typedef std::vector<std::unique_ptr<VulkanCommandBuffer>> CommandBufferList;

		vk::UniqueCommandPool command_pool;
		std::array<CommandBufferList, caps::MAX_FRAMES_IN_FLIGHT> allocated_command_buffers;
		uint32_t current_frame_allocated_buffers;
		uint32_t current_frame;
	};

	struct VulkanImageInitialzier
	{

	};

	class VulkanImage
	{
	public:

	};

}