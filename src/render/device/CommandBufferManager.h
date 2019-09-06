#pragma once

#include "CommonIncludes.h"

namespace core { namespace Device {

	class VulkanCommandPool;

	class CommandBufferManager
	{
	public:
		CommandBufferManager(uint32_t frame_count);
		virtual ~CommandBufferManager();
		VulkanCommandPool* GetDefaultCommandPool();

	private:
		static thread_local std::unique_ptr<VulkanCommandPool> default_command_pool;
	};

} }