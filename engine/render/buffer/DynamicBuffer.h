#pragma once

#include "CommonIncludes.h"
#include "VulkanBuffer.h"
#include "render/device/VulkanUploader.h"
#include "render/device/VulkanContext.h"
#include "Engine.h"

namespace Device {

	template<typename T>
	class DynamicBuffer
	{
	public:
		DynamicBuffer(std::string name, size_t size, BufferType type = BufferType::Uniform, bool align = true, void* data = nullptr)
			: name(name), size(size), type(type), alignment(align ? 256 : 1) // TODO: get from API
		{
			assert(size >= sizeof(T) && "buffer size must be greater than the element size");
			auto main_initializer = VulkanBufferInitializer(size).Name("Dynamic " + name).Data(data);

			switch (type)
			{
			case BufferType::Uniform:
				main_initializer.SetUniform();
				break;

			case BufferType::Storage:
				main_initializer.SetStorage();
				break;

			case BufferType::Indirect:
				main_initializer.SetIndirect();
				break;

			case BufferType::Vertex:
				main_initializer.SetVertex();
				break;

			case BufferType::Index:
				main_initializer.SetIndex();
				break;
			}

			buffer = VulkanBuffer::Create(main_initializer);

			auto staging_initializer = VulkanBufferInitializer(size).SetStaging().Name("StagingDynamic " + name);
			for (int i = 0; i < staging_buffers.size(); i++)
				staging_buffers[i] = VulkanBuffer::Create(staging_initializer);
		}

		const std::string& GetName() const { return name; }
		BufferType GetType() const { return type; }
		VulkanBuffer* GetBuffer() const { return buffer.get(); }
		size_t GetElementSize() const { return sizeof(T); }

		void* Map()
		{
			mapped_pointer = staging_buffers[current_staging_buffer]->Map();
			return mapped_pointer;
		}

		void Unmap()
		{
			staging_buffers[current_staging_buffer]->Unmap();

			if (upload_data_size)
			{
				auto* uploader = Engine::GetVulkanContext()->GetUploader();
				uploader->AddToUpload(staging_buffers[current_staging_buffer], buffer.get(), upload_data_size);
			}

			current_staging_buffer = (current_staging_buffer + 1) % staging_buffers.size();
			upload_data_size = 0;
			mapped_pointer = nullptr;
		}

		size_t Append(const T& element)
		{
			if (!mapped_pointer)
				throw std::runtime_error("Buffer should be mapped");
			if (upload_data_size + sizeof(T) > size)
				throw std::runtime_error("Buffer overflow");

			auto result = upload_data_size;
			
			memcpy((char*)mapped_pointer + upload_data_size, &element, sizeof(T));
			SetUploadSize(
				(std::min)(((size_t)ceilf((upload_data_size + sizeof(T)) / (float)alignment) * alignment), size)
			);

			return result;
		}

		size_t GetSize() const { return size; }

		void SetUploadSize(size_t data_size)
		{
			assert(data_size <= size);
			upload_data_size = std::min(data_size, size);
		}

	protected:
		std::string name;
		void* mapped_pointer = nullptr;
		size_t upload_data_size = 0;
		size_t size = 0;
		size_t alignment;
		unsigned current_staging_buffer = 0;
		VulkanBuffer::Handle buffer;
		std::array<VulkanBuffer::Handle, 3> staging_buffers;
		BufferType type;
	};

}