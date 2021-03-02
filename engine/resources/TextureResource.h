#pragma once

#include "ResourceCache.h"
#include "render/device/Resource.h"
#include <string>

namespace Device
{
	class Texture;
}

namespace Resources
{
	class TextureResource
	{
	public:
		using Handle = Handle<TextureResource>;

		struct Initializer
		{
			static constexpr uint32_t FLAG_SRGB = 1 << 0;

			Initializer(const std::wstring& filename) : filename(filename) {}
			Initializer(const Initializer&) = default;

			const std::wstring& GetPath() const { return filename; }
			const uint32_t GetHash() const 
			{
				uint32_t hashes[] = { FastHash(filename), flags };
				return FastHash(hashes, sizeof(hashes));
			}

			Initializer& SetSRGB(bool value)
			{
				if (value)
					flags |= FLAG_SRGB;
				else
					flags &= ~FLAG_SRGB;

				return *this;
			}

			bool GetSRGB() const { return flags & FLAG_SRGB; }

		private:
			std::wstring filename;
			uint32_t flags = 0;
		};

		TextureResource(const std::wstring& filename);
		TextureResource(const Initializer& initializer);
		~TextureResource();

		static Handle SRGB(const std::wstring& filename)
		{
			return Handle(Initializer(filename).SetSRGB(true));
		}

		static Handle Linear(const std::wstring& filename)
		{
			return Handle(Initializer(filename).SetSRGB(false));
		}

		const Device::Handle<Device::Texture>& Get() const { return texture; }

	private:
		Device::Handle<Device::Texture> texture;
	};

}