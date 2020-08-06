#include "VulkanBuffer.h"
#include "Engine.h"
#include "render/device/VulkanContext.h"
#include "render/device/VulkanUploader.h"
#include "render/device/VkObjects.h"
#include "render/device/VulkanUtils.h"

namespace Device {

	VulkanBuffer::VulkanBuffer(VulkanBufferInitializer initializer)
	{
		usage = initializer.usage;
		size = initializer.size;

		VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = initializer.memory_usage;
		
		auto allocator = Engine::GetVulkanContext()->GetAllocator();
		vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr);

		if (initializer.data)
		{
			if (usage & VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
			{
				// Write data to staging buffer
				void* staging_data = Map();
				memcpy(staging_data, initializer.data, (size_t)size);
				Unmap();
			}
			else {

				// Create temporary staging buffer
				auto staging_buffer = VulkanBuffer::Create(
					VulkanBufferInitializer(size).SetStaging().Data(initializer.data)
				);

				// Upload from staging to current buffer
				auto* uploader = Engine::GetVulkanContext()->GetUploader();
				uploader->AddToUpload(staging_buffer.get(), this, size);
			}

		}
	}

	VulkanBuffer::~VulkanBuffer()
	{
		auto allocator = Engine::GetVulkanContext()->GetAllocator();
		vmaDestroyBuffer(allocator, buffer, allocation);
	}

	void* VulkanBuffer::Map()
	{
		auto allocator = Engine::GetVulkanContext()->GetAllocator();

		void* data;
		vmaMapMemory(allocator, allocation, &data);
		return data;
	}

	void VulkanBuffer::Unmap()
	{
		auto allocator = Engine::GetVulkanContext()->GetAllocator();
		vmaUnmapMemory(allocator, allocation);
	}

}