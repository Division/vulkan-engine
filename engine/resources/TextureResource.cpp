#include "TextureResource.h"
#include "render/texture/Texture.h"
#include "loader/TextureLoader.h"

namespace Resources
{
	TextureResource::TextureResource(const std::wstring& filename)
	{
		texture = Device::Texture::Handle(loader::LoadTexture(filename));
	}

	TextureResource::TextureResource(const Initializer& initializer)
	{
		texture = Device::Texture::Handle(loader::LoadTexture(initializer.GetPath(), initializer.GetSRGB()));
	}

	TextureResource::~TextureResource() = default;
}