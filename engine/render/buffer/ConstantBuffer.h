#pragma once

#include "VulkanBuffer.h"
#include "DynamicBuffer.h"
#include "utils/Math.h"

namespace Device {

	class ConstantBuffer : protected DynamicBuffer<uint8_t>
	{
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
			if (upload_data_size)
			{
				DynamicBuffer::Unmap();
				DynamicBuffer::Map();
			}
		}

		size_t GetSize() const { return DynamicBuffer::GetSize(); }

		Allocation Allocate(uint32_t allocation_size)
		{
			assert(mapped_pointer);
			size_t pointer = AlignMemory((size_t)mapped_pointer + upload_data_size, 256);
			upload_data_size = pointer - (size_t)mapped_pointer + allocation_size;
			if (upload_data_size > size)
				throw std::runtime_error("Constant buffer overflow");

			return { (uint8_t*)pointer, (uint32_t)((size_t)pointer - (size_t)mapped_pointer), allocation_size };
		}

	};

	class DynamicParameters
	{
		ConstantBuffer& constant_buffer;
		uint32_t size;
		ConstantBuffer::Allocation allocation;

	public:
		DynamicParameters(ConstantBuffer& constant_buffer, uint32_t size)
			: constant_buffer(constant_buffer)
			, size(size)
		{}

		void Patch(void* src, uint32_t patching_size)
		{
			if (patching_size > size)
				throw std::runtime_error("constant bufer size is too low");
			allocation = constant_buffer.Allocate(size);
			memcpy(allocation.pointer, src, patching_size);
		}

	};

}