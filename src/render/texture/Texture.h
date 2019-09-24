#pragma once

#include "CommonIncludes.h"

namespace core { namespace Device {
	
	struct TextureInitializer
	{
		enum Mode
		{
			RAW_RGB_A
		};

		TextureInitializer(uint32_t width, uint32_t height, uint32_t channel_count, void* data, bool sRGB) // simple RGBA
			: mode(RAW_RGB_A), channel_count(channel_count), width(width), height(height), data(data), sRGB(sRGB) {}

		Mode mode;
		bool sRGB = false;
		void* data = nullptr;
		uint32_t channel_count = 0;
		size_t data_size = 0;
		bool is_from_file = false;
		uint32_t width = 0;
		uint32_t height = 0;
	};

	class Texture {
	public:
		Texture(const TextureInitializer& initializer);
		~Texture();

		const vk::Image& GetImage() const { return image; }
		const vk::ImageView& GetImageView() const { return image_view.get(); }

	private:
		VmaAllocation allocation;
		vk::Image image;
		vk::UniqueImageView image_view;

	};

} }
