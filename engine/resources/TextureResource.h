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

		TextureResource(const std::wstring& filename);
		~TextureResource();

		Device::Handle<Device::Texture> Get() const { return texture; }

	private:
		Device::Handle<Device::Texture> texture;
	};
}