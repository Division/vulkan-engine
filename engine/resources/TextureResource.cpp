#include "TextureResource.h"
#include "render/texture/Texture.h"
#include "loader/TextureLoader.h"

namespace Resources
{
	TextureResource::TextureResource(const std::wstring& filename)
	{
		texture = Device::Texture::Handle(loader::LoadTexture(filename));
	}

	TextureResource::~TextureResource() = default;
}