#pragma once

#include "CommonIncludes.h"
#include "render/device/VulkanCaps.h"
#include "render/device/Types.h"
#include "render/device/Resource.h"

namespace Device {
	
	class VulkanBuffer;

	struct TextureInitializer
	{
		enum Mode
		{
			Default,
			DepthBuffer,
			ColorTarget
		};

		TextureInitializer(uint32_t width, uint32_t height, uint32_t channel_count, void* data, bool sRGB) // simple RGBA
			: width(width), height(height), data(data), sRGB(sRGB) 
		{
			assert(channel_count == 4);
			format = sRGB ? Format::R8G8B8A8_srgb : Format::R8G8B8A8_unorm;
			data_size = width * height * depth * (uint32_t)format_sizes.at(format) / 8;
		}

		TextureInitializer(uint32_t width, uint32_t height, uint32_t depth, uint32_t array_layers, Format format, uint32_t mip_levels = 1)
			: width(width), height(height), depth(depth), array_layers(array_layers), format(format), mip_levels(mip_levels) {}

		TextureInitializer& TextureInitializer::SetData(void* data, size_t size)
		{
			this->data = data;
			this->data_size = size;
			return *this;
		}

		TextureInitializer& TextureInitializer::SetDepth()
		{
			mode = DepthBuffer;
			return *this;
		}

		TextureInitializer& TextureInitializer::SetSampled()
		{
			force_sampled = true;
			return *this;
		}

		TextureInitializer& TextureInitializer::SetColorTarget()
		{
			mode = ColorTarget;
			return *this;
		}

		TextureInitializer& TextureInitializer::SetNumDimensions(int dimensions)
		{
			num_dimensions = dimensions;
			return *this;
		}

		TextureInitializer& TextureInitializer::SetCube(bool value)
		{
			is_cube = value;
			return *this;
		}

		TextureInitializer& TextureInitializer::SetArray(bool value)
		{
			is_array = value;
			return *this;
		}

		TextureInitializer& TextureInitializer::AddCopy(vk::BufferImageCopy copy)
		{
			copies.push_back(copy);
			return *this;
		}


		// Allows mapping and dynamic uploading
		TextureInitializer& SetDynamic()
		{
			dynamic = true;
			return *this;
		}

		bool dynamic = false;
		Mode mode = Default;
		bool sRGB = false;
		void* data = nullptr;
		bool force_sampled = false;
		uint32_t channel_count = 0;
		size_t data_size = 0;
		bool is_from_file = false;
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t depth = 1;
		uint32_t array_layers = 1;
		uint32_t mip_levels = 1;
		Format format = Format::Undefined;
		std::vector<vk::BufferImageCopy> copies;
		uint32_t num_dimensions = 2;
		bool is_array = false;
		bool is_cube = false;
		std::string name;
	};

	class Texture : public Resource {
	public:
		using Handle = Handle<Texture>;

		static Texture::Handle Create(const TextureInitializer& initializer);

		Texture(const TextureInitializer& initializer);
		~Texture();

		const vk::Image& GetImage() const { return image; }
		const vk::ImageView& GetImageView() const { return image_view.get(); }
		void* Texture::Map();
		void Texture::Unmap();
		Format GetFormat() const { return format; }
		uint32_t GetSampleCount() const { return 1; /* not implemented */ }
		uint32_t GetHash() const;

	private:
		void SetupRGBA(const TextureInitializer& initializer);
		void SetupDefault(const TextureInitializer& initializer);
		std::vector<vk::BufferImageCopy> GetCopies();

	private:
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t depth = 0;
		uint32_t array_layers = 0;
		uint32_t mip_levels = 0;
		vk::DeviceSize size;
		Format format;
		VmaAllocation allocation;
		vk::Image image;
		vk::UniqueImageView image_view;
		bool dynamic;
		std::array<Device::Handle<VulkanBuffer>, caps::MAX_FRAMES_IN_FLIGHT> staging_buffers;
		uint32_t current_staging_buffer = 0;
		void* mapped_pointer = nullptr;
	};

}
