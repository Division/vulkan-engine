#pragma once

#include "CommonIncludes.h"
#include "VulkanBuffer.h"
#include "render/device/VulkanCaps.h"
#include "render/device/VulkanUploader.h"
#include "Engine.h"

namespace core { namespace Device {

	template<typename T>
	class UniformBuffer
	{
	public:
		UniformBuffer(size_t size, bool ssbo = false, bool align = true)
			: size(size), alignment(align ? 256 : 1) // TODO: get from API
		{
			assert(size >= sizeof(T) && "buffer size must be greater than the element size");
			auto main_initializer = VulkanBufferInitializer(size);
			if (ssbo)
				main_initializer.SetStorage();
			else
				main_initializer.SetUniform();

			buffer = std::make_unique<VulkanBuffer>(main_initializer);

			auto staging_initializer = VulkanBufferInitializer(size).SetStaging();
			for (int i = 0; i < staging_buffers.size(); i++)
				staging_buffers[i] = std::make_unique<VulkanBuffer>(staging_initializer);
		}

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
			auto* uploader = Engine::GetVulkanContext()->GetUploader();
			uploader->AddToUpload(staging_buffers[current_staging_buffer].get(), buffer.get(), size);

			current_staging_buffer = (current_staging_buffer + 1) % caps::MAX_FRAMES_IN_FLIGHT;
			frame_data_size = 0;
			mapped_pointer = nullptr;
		}

		size_t Append(const T& element)
		{
			if (!mapped_pointer)
				throw std::runtime_error("Buffer should be mapped");
			if (frame_data_size + sizeof(T) > size)
				throw std::runtime_error("Buffer overflow");

			auto result = frame_data_size;
			
			memcpy((char*)mapped_pointer + frame_data_size, &element, sizeof(T));
			SetFrameDataSize(
				std::min(((size_t)ceilf((frame_data_size + sizeof(T)) / (float)alignment) * alignment), size)
			);

			return result;
		}

		size_t GetSize() const { return size; }

		void SetFrameDataSize(size_t data_size)
		{
			assert(data_size <= size);
			frame_data_size = std::min(data_size, size);
		}

	private:
		void* mapped_pointer = nullptr;
		size_t frame_data_size = 0;
		size_t size = 0;
		size_t alignment;
		unsigned current_staging_buffer = 0;
		std::unique_ptr<VulkanBuffer> buffer;
		std::array<std::unique_ptr<VulkanBuffer>, caps::MAX_FRAMES_IN_FLIGHT> staging_buffers;
	};

} }