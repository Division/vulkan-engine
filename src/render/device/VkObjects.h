#pragma once

#include "CommonIncludes.h"
#include "VulkanCaps.h"

namespace core { namespace Device {
	
	class VulkanCommandPool;

	class VulkanCommandBuffer : NonCopyable
	{
	public:
		explicit VulkanCommandBuffer(VulkanCommandPool* command_pool);
		explicit VulkanCommandBuffer(VulkanCommandBuffer&& other);
		~VulkanCommandBuffer();

		VkCommandBuffer GetCommandBuffer() { return command_buffer; }
		VulkanCommandPool* GetCommandPool() { return command_pool; }

	private:
		VkCommandBuffer command_buffer = VK_NULL_HANDLE;
		VulkanCommandPool* command_pool = nullptr;
	};


	class VulkanCommandPool : NonCopyable
	{
	public:
		VulkanCommandPool();
		virtual ~VulkanCommandPool();

		VkCommandPool GetCommandPool() const { return command_pool; }
		VulkanCommandBuffer* GetCommandBuffer();
		void NextFrame();

	private:
		typedef std::vector<std::unique_ptr<VulkanCommandBuffer>> CommandBufferList;

		VkCommandPool command_pool = VK_NULL_HANDLE;
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

} }