#include "CommandBufferManager.h"
#include "VkObjects.h"

namespace core { namespace Device {

	thread_local std::unique_ptr<VulkanCommandPool> CommandBufferManager::default_command_pool;

	CommandBufferManager::CommandBufferManager(uint32_t frame_count)
	{

	}

	CommandBufferManager::~CommandBufferManager()
	{
		default_command_pool = nullptr;
	}

	VulkanCommandPool* CommandBufferManager::GetDefaultCommandPool()
	{
		if (!default_command_pool)
			default_command_pool = std::make_unique<VulkanCommandPool>();

		return default_command_pool.get();
	}

} }