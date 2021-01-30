#pragma once

#include "ResourceCache.h"
#include "render/material/Material.h"
#include "resources/TextureResource.h"
#include <string>

namespace Resources
{
	class MaterialResource
	{
	public:
		using Handle = Handle<MaterialResource>;

		MaterialResource(const std::wstring& filename);
		~MaterialResource();

		const Common::Handle<Material> Get() const { return material; }

	private:
		Common::Handle<Material> material;

	};
}

namespace render
{
	class MaterialResourceList : public std::vector<Resources::MaterialResource::Handle>, public Common::Resource
	{
	public:
		using Handle = std::shared_ptr<MaterialResourceList>;

		static Handle Create()
		{
			return Handle(std::make_unique<MaterialResourceList>());
		}

		~MaterialResourceList()
		{
		}
	};
}