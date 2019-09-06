#include "VkObjects.h"
#include "Engine.h"
#include "VulkanUtils.h"
#include "VulkanContext.h"

namespace core { namespace Device {
	
	VulkanCommandBuffer::VulkanCommandBuffer(VulkanCommandPool* command_pool)
	{
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = command_pool->GetCommandPool();
		allocInfo.commandBufferCount = 1;

		this->command_pool = command_pool;
		vkAllocateCommandBuffers(Engine::GetVulkanDevice(), &allocInfo, &command_buffer);
	}

	VulkanCommandBuffer::VulkanCommandBuffer(VulkanCommandBuffer&& other)
	{
		command_buffer = other.command_buffer;
		command_pool = other.command_pool;
		other.command_buffer = VK_NULL_HANDLE;
		other.command_pool = VK_NULL_HANDLE;
	}

	VulkanCommandBuffer::~VulkanCommandBuffer()
	{
		if (command_buffer != VK_NULL_HANDLE)
		{
			vkFreeCommandBuffers(Engine::GetVulkanDevice(), command_pool->GetCommandPool(), 1, &command_buffer);
		}
	}

	void VulkanCommandBuffer::Release()
	{
		command_pool->Release(this);
	}

	//-----------------------------------------------------------------------------

	// TODO: queueFamilyIndex as parameter
	VulkanCommandPool::VulkanCommandPool()
	{
		VulkanUtils::QueueFamilyIndices queueFamilyIndices = VulkanUtils::FindQueueFamilies(
			Engine::GetVulkanContext()->GetPhysicalDevice(), Engine::GetVulkanContext()->GetSurface());

		VkCommandPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		pool_info.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

		if (vkCreateCommandPool(Engine::GetVulkanDevice(), &pool_info, nullptr, &command_pool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics command pool!");
		}
	}

	VulkanCommandPool::~VulkanCommandPool()
	{
		allocated_command_buffers.clear();
		if (command_pool != VK_NULL_HANDLE)
		{
			vkDestroyCommandPool(Engine::GetVulkanDevice(), command_pool, nullptr);
		}
	}

	VulkanCommandBuffer* VulkanCommandPool::GetCommandBuffer()
	{
		allocated_command_buffers.push_back(std::make_unique<VulkanCommandBuffer>(this));
		return allocated_command_buffers[allocated_command_buffers.size() - 1].get();
	}

	void VulkanCommandPool::Release(VulkanCommandBuffer* buffer)
	{
		auto new_end = std::remove_if(
			allocated_command_buffers.begin(), allocated_command_buffers.end(), 
			[&](std::unique_ptr<VulkanCommandBuffer>& current_buffer) {
				return buffer == current_buffer.get();
			}
		);

		allocated_command_buffers.erase(new_end, allocated_command_buffers.end());
	}

	void VulkanCommandPool::Reset()
	{
		allocated_command_buffers.clear();
	}

} }