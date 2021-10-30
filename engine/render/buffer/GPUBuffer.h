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
		GPUBuffer(std::string name, size_t size, BufferType type = BufferType::Uniform, void* data = nullptr)
			: name(name), size(size), type(type)
		{
			auto initializer = VulkanBufferInitializer(size).Name("GPU Buffer " + name).Data(data);

			switch (type)
			{
			case BufferType::Uniform:
				initializer.SetUniform();
				break;

			case BufferType::Storage:
				initializer.SetStorage();
				break;

			case BufferType::Vertex:
				initializer.SetVertex();
				break;

			case BufferType::Index:
				initializer.SetIndex();
				break;
			}

			buffer = VulkanBuffer::Create(initializer);
		}

		const std::string& GetName() const { return name; }
		BufferType GetType() const { return type; }
		const VulkanBuffer::Handle& GetBuffer() const { return buffer; }
		size_t GetSize() const { return size; }

	protected:
		std::string name;
		size_t size = 0;
		VulkanBuffer::Handle buffer;
		BufferType type;
	};

}