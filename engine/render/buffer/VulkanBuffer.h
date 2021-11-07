#pragma once
#include "CommonIncludes.h"
#include "render/device/Resource.h"

namespace Device {

	enum BufferType : int
	{
		Uniform,
		Storage,
		Indirect,
		Vertex,
		Index
	};

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

		VulkanBufferInitializer& SetStorage()
		{
			usage = 0;
			return MemoryUsage(VMA_MEMORY_USAGE_GPU_ONLY)
				.Usage(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		}

		VulkanBufferInitializer& SetIndirect()
		{
			usage = 0;
			return MemoryUsage(VMA_MEMORY_USAGE_GPU_ONLY)
				.Usage(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
		}

		VulkanBufferInitializer& SetStaging()
		{
			usage = 0;
			return MemoryUsage(VMA_MEMORY_USAGE_CPU_ONLY).Usage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
		}

		VulkanBufferInitializer& SetVertex()
		{
			return MemoryUsage(VMA_MEMORY_USAGE_GPU_ONLY).Usage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
		}

		VulkanBufferInitializer& SetIndex()
		{
			return MemoryUsage(VMA_MEMORY_USAGE_GPU_ONLY).Usage(VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
		}

		VulkanBufferInitializer& MemoryUsage(VmaMemoryUsage memory_usage)
		{
			this->memory_usage = memory_usage;
			return *this;
		}

		VulkanBufferInitializer& Data(const void* data)
		{
			this->data = data;
			return *this;
		}

		VulkanBufferInitializer& Name(std::string name)
		{
			this->debug_name = std::move(name);
			return *this;
		}

		std::string debug_name;
		VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		uint32_t size = 0;
		VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_UNKNOWN;
		const void* data = nullptr;
	};

	class VulkanBuffer : public ::Device::Resource {
	public:
		VulkanBuffer(VulkanBufferInitializer initializer);
		~VulkanBuffer();

		using Handle = ::Device::Handle<VulkanBuffer>;

		static ::Device::Handle<VulkanBuffer> Create(const VulkanBufferInitializer& initializer) 
		{ 
			return Handle(std::make_unique<VulkanBuffer>(initializer)); 
		}

		static Handle Create(const VulkanBufferInitializer&& initializer) { return Handle(std::make_unique<VulkanBuffer>(initializer)); }

		uint32_t Size() const { return size; }
		VkBuffer Buffer() const { return buffer; }
		
		void* Map();
		void Unmap();
		const std::string& GetDebugName() const { return debug_name; }

	protected:
		std::string debug_name;
		VkBuffer buffer;
		VkBufferUsageFlags usage;
		uint32_t size;

		VmaAllocation allocation;
	};

}