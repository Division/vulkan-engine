#include "VulkanBuffer.h"
#include "Engine.h"
#include "render/device/Device.h"
#include "render/device/VulkanContext.h"
#include "render/device/VulkanUploader.h"
#include "render/device/CommandBufferManager.h"
#include "render/device/VkObjects.h"
#include "render/device/VulkanUtils.h"

namespace core { namespace Device {

	VulkanBuffer::VulkanBuffer(VulkanBufferInitializer initializer)
	{
		usage = initializer.usage;
		size = initializer.size;

		VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = initializer.memory_usage;
		
		auto allocator = Engine::Get()->GetDevice()->GetContext()->GetAllocator();
		vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr);

		if (initializer.data)
		{
			auto* uploader = Engine::GetVulkanContext()->GetUploader();
			//VulkanBuffer staging_buffer(VulkanBufferInitializer(size).SetStaging());

			// Create temporary staging buffer
			auto staging_buffer = std::make_unique<VulkanBuffer>(VulkanBufferInitializer(size).SetStaging());

			// Write data to staging buffer
			void* staging_data = staging_buffer->Map();
			memcpy(staging_data, initializer.data, (size_t)size);
			staging_buffer->Unmap();

			uploader->AddToUpload(std::move(staging_buffer), this, size);
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
		auto allocator = Engine::Get()->GetDevice()->GetContext()->GetAllocator();
		vmaUnmapMemory(allocator, allocation);
	}

} }