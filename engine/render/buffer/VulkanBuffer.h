#pragma once
#include "CommonIncludes.h"

namespace core { namespace Device {

	struct VulkanBufferInitializer {

		VulkanBufferInitializer(uint32_t size) : size(size) {}

		VulkanBufferInitializer& Usage(VkBufferUsageFlags usage) 
		{
			this->usage |= usage;
			return *this;
		}

		VulkanBufferInitializer& SetUniform()
		{
			usage = 0;
			return MemoryUsage(VMA_MEMORY_USAGE_GPU_ONLY)
					 .Usage(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
		}

		VulkanBufferInitializer& SetStaging()
		{
			usage = 0;
			return MemoryUsage(VMA_MEMORY_USAGE_CPU_ONLY).Usage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
		}

		VulkanBufferInitializer& SetVertex()
		{
			return Usage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
		}

		VulkanBufferInitializer& SetIndex()
		{
			return Usage(VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
		}

		VulkanBufferInitializer& MemoryUsage(VmaMemoryUsage memory_usage)
		{
			this->memory_usage = memory_usage;
			return *this;
		}

		VulkanBufferInitializer& Data(void* data)
		{
			this->data = data;
			return *this;
		}

		VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		uint32_t size = 0;
		VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_UNKNOWN;
		void* data = nullptr;
	};

	class VulkanBuffer {
	public:
		VulkanBuffer(VulkanBufferInitializer initializer);
		~VulkanBuffer();

		uint32_t Size() const { return size; }
		VkBuffer Buffer() const { return buffer; }
		
		void *Map();
		void Unmap();

	protected:
		VkBuffer buffer;
		VkBufferUsageFlags usage;
		uint32_t size;

		VmaAllocation allocation;
	};

} }