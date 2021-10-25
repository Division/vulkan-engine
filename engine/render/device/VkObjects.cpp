#include "VkObjects.h"
#include "Engine.h"
#include "VulkanUtils.h"
#include "VulkanContext.h"

namespace Device {
	
	VulkanCommandBuffer::VulkanCommandBuffer(VulkanCommandPool* command_pool)
	{
		vk::CommandBufferAllocateInfo alloc_info(command_pool->GetCommandPool(), vk::CommandBufferLevel::ePrimary, 1);

		this->command_pool = command_pool;
		auto device = (vk::Device)Engine::GetVulkanDevice();
		command_buffer = device.allocateCommandBuffers(alloc_info)[0];
		name = command_pool->GetName();
		if (name.size())
			Engine::GetVulkanContext()->AssignDebugName((uint64_t)(VkCommandBuffer)command_buffer, vk::DebugReportObjectTypeEXT::eCommandBuffer, ("[CommandBuffer] " + name).c_str());
	}

	VulkanCommandBuffer::VulkanCommandBuffer(VulkanCommandBuffer&& other)
	{
		command_buffer = other.command_buffer;
		command_pool = other.command_pool;
		name = std::move(name);
		other.command_buffer = vk::CommandBuffer();
		other.command_pool = nullptr;
	}

	VulkanCommandBuffer::~VulkanCommandBuffer()
	{
		Engine::GetVulkanDevice().freeCommandBuffers(command_pool->GetCommandPool(), 1, &command_buffer);
	}

	//-----------------------------------------------------------------------------

	// TODO: queueFamilyIndex as parameter
	VulkanCommandPool::VulkanCommandPool(uint32_t queue_family, const std::string name)
		: current_frame_allocated_buffers(0)
		, name(std::move(name))
	{
		vk::CommandPoolCreateInfo pool_info({ vk::CommandPoolCreateFlagBits::eResetCommandBuffer }, queue_family);
		command_pool = Engine::GetVulkanDevice().createCommandPoolUnique(pool_info);
		if (name.size())
			Engine::GetVulkanContext()->AssignDebugName((uint64_t)(VkCommandPool)command_pool.get(), vk::DebugReportObjectTypeEXT::eCommandPool, ("[CommandPool] " + name).c_str());
	}

	VulkanCommandPool::~VulkanCommandPool()
	{
		for (int i = 0; i < caps::MAX_FRAMES_IN_FLIGHT; i++)
		{
			allocated_command_buffers[i].clear();
		}
	}

	VulkanCommandBuffer* VulkanCommandPool::GetCommandBuffer()
	{
		VulkanCommandBuffer* result;
		auto& list = allocated_command_buffers[current_frame % caps::MAX_FRAMES_IN_FLIGHT];
		if (list.size() > current_frame_allocated_buffers)
		{
			result = list[current_frame_allocated_buffers].get();
		}
		else 
		{
			list.push_back(std::make_unique<VulkanCommandBuffer>(this));
			result = list.back().get();
		}
		
		current_frame_allocated_buffers += 1;
		return result;
	}

	void VulkanCommandPool::NextFrame()
	{
		current_frame += 1;
		current_frame_allocated_buffers = 0;
	}

}