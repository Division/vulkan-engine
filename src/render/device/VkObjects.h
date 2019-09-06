#pragma once

#include "CommonIncludes.h"

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

		void Release();

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
		void Reset();
		void Release(VulkanCommandBuffer* buffer);

	private:
		VkCommandPool command_pool = VK_NULL_HANDLE;
		std::vector<std::unique_ptr<VulkanCommandBuffer>> allocated_command_buffers;
	};

} }