#pragma once

#include "VulkanBuffer.h"
#include "DynamicBuffer.h"
#include "utils/Math.h"

namespace Device {

	class ConstantBuffer : protected DynamicBuffer<uint8_t>
	{
		std::mutex mutex;

	public:
		struct Allocation
		{
			uint8_t* pointer = nullptr;
			uint32_t offset = 0;
			uint32_t size = 0;
		};

		ConstantBuffer(size_t size, BufferType type = BufferType::Uniform)
			: DynamicBuffer<uint8_t>(size, type, false)
		{
			DynamicBuffer::Map();
		}

		~ConstantBuffer()
		{
			DynamicBuffer::Unmap();
		}

		VulkanBuffer* GetBuffer() const { return DynamicBuffer::GetBuffer(); }

		void Upload()
		{
			std::scoped_lock lock(mutex);

			if (upload_data_size)
			{
				DynamicBuffer::Unmap();
				DynamicBuffer::Map();
			}
		}

		size_t GetSize() const { return DynamicBuffer::GetSize(); }

		Allocation Allocate(uint32_t allocation_size)
		{
			// May be a lot of contention here. Consider spinlock if need to render a lot.
			std::scoped_lock lock(mutex);

			assert(mapped_pointer);
			size_t pointer = AlignMemory((size_t)mapped_pointer + upload_data_size, 256);
			upload_data_size = pointer - (size_t)mapped_pointer + allocation_size;
			if (upload_data_size > size)
				throw std::runtime_error("Constant buffer overflow");

			return { (uint8_t*)pointer, (uint32_t)((size_t)pointer - (size_t)mapped_pointer), allocation_size };
		}

	};

}