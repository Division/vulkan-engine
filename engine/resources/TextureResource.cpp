#include "TextureResource.h"
#include "render/texture/Texture.h"
#include "loader/TextureLoader.h"
#include "system/Dialogs.h"
#include "utils/StringUtils.h"

#define LOG_ERRORS 1

namespace Resources
{
	void TextureResource::HandleLoadingError(const wchar_t* path)
	{
		uint32_t data = 0xFFFF00FF;
		Device::TextureInitializer initializer(1, 1, 4, &data, false);
		texture = std::make_unique<Device::Texture>(initializer);

#if (LOG_ERRORS)
		System::EnqueueMessage("Error loading texture", utils::WStringToString(path).c_str());
#endif
	}

	TextureResource::TextureResource(const std::wstring& filename)
	{
		texture = Device::Texture::Handle(loader::LoadTexture(filename));
		if (!texture)
			HandleLoadingError(filename.c_str());
	}

	TextureResource::TextureResource(const Initializer& initializer)
	{
		texture = Device::Texture::Handle(loader::LoadTexture(initializer.GetPath(), initializer.GetSRGB()));
		if (!texture)
			HandleLoadingError(initializer.GetPath());
	}

	TextureResource::~TextureResource() = default;
}