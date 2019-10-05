#include "VkObjects.h"
#include "Engine.h"
#include "VulkanUtils.h"
#include "VulkanContext.h"

namespace core { namespace Device {
	
	VulkanCommandBuffer::VulkanCommandBuffer(VulkanCommandPool* command_pool)
	{
		vk::CommandBufferAllocateInfo alloc_info(command_pool->GetCommandPool(), vk::CommandBufferLevel::ePrimary, 1);

		this->command_pool = command_pool;
		auto device = (vk::Device)Engine::GetVulkanDevice();
		command_buffer = device.allocateCommandBuffers(alloc_info)[0];
	}

	VulkanCommandBuffer::VulkanCommandBuffer(VulkanCommandBuffer&& other)
	{
		command_buffer = other.command_buffer;
		command_pool = other.command_pool;
		other.command_buffer = vk::CommandBuffer();
		other.command_pool = nullptr;
	}

	VulkanCommandBuffer::~VulkanCommandBuffer()
	{
		Engine::GetVulkanDevice().freeCommandBuffers(command_pool->GetCommandPool(), 1, &command_buffer);
	}

	//-----------------------------------------------------------------------------

	// TODO: queueFamilyIndex as parameter
	VulkanCommandPool::VulkanCommandPool()
		: current_frame_allocated_buffers(0)
	{
		VulkanUtils::QueueFamilyIndices queueFamilyIndices = VulkanUtils::FindQueueFamilies(
			Engine::GetVulkanContext()->GetPhysicalDevice(), Engine::GetVulkanContext()->GetSurface());

		vk::CommandPoolCreateInfo pool_info({ vk::CommandPoolCreateFlagBits::eResetCommandBuffer }, queueFamilyIndices.graphicsFamily.value());

		command_pool = Engine::GetVulkanDevice().createCommandPoolUnique(pool_info);
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

} }