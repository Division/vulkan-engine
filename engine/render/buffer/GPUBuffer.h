#pragma once

#include "CommonIncludes.h"
#include "VulkanBuffer.h"
#include "render/device/VulkanUploader.h"
#include "render/device/VulkanContext.h"
#include "Engine.h"

namespace Device {

	// Buffer that is accessible only by GPU (doesn't have staging)
	class GPUBuffer : public NonCopyable
	{
	public:
		GPUBuffer(std::string name, size_t size, BufferType type = BufferType::Uniform, const void* data = nullptr)
			: name(name), size(size), type(type)
		{
			auto initializer = VulkanBufferInitializer(size).Name("GPU Buffer " + name).Data(data);

			const uint32_t type_uint = (uint32_t)type;

			if (type_uint & (uint32_t)BufferType::Uniform)
				initializer.SetUniform();
			else if (type_uint & (uint32_t)BufferType::Storage)
				initializer.SetStorage();
			else if (type_uint & (uint32_t)BufferType::Indirect)
				initializer.SetIndirect();
			else if (type_uint & (uint32_t)BufferType::Vertex)
				initializer.SetVertex();
			else if (type_uint & (uint32_t)BufferType::Index)
				initializer.SetIndex();

			if (type_uint & (uint32_t)BufferType::TransferSrc)
				initializer.Usage(VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
			if (type_uint & (uint32_t)BufferType::TransferDst)
				initializer.Usage(VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT);

			buffer = VulkanBuffer::Create(initializer);
		}

		const std::string& GetName() const { return name; }
		BufferType GetType() const { return type; }
		const VulkanBuffer::Handle& GetBuffer() const { return buffer; }
		uint32_t GetSize() const { return size; }

	protected:
		std::string name;
		uint32_t size = 0;
		VulkanBuffer::Handle buffer;
		BufferType type;
	};

}