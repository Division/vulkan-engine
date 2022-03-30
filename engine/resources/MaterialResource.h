#pragma once

#include "ResourceCache.h"
#include "render/material/Material.h"
#include "resources/TextureResource.h"
#include <string>
#include <variant>

namespace Resources
{
	class MaterialResource
	{
	public:
		using Handle = Handle<MaterialResource>;

		MaterialResource(const std::wstring& filename);
		~MaterialResource();

		const Common::Handle<Material>& Get() const { return material; }

	private:
		Common::Handle<Material> material;

	};
}

namespace render
{
	class MaterialUnion
	{
		std::variant<Resources::MaterialResource::Handle, Material::Handle> material;

	public:
		MaterialUnion() : material(Material::Handle()) {}
		MaterialUnion(Resources::MaterialResource::Handle material) : material(material) {}
		MaterialUnion(Material::Handle material) : material(material) {}

		MaterialUnion& operator=(const Resources::MaterialResource::Handle& material)
		{
			this->material = material;
			return *this;
		}

		MaterialUnion& operator=(const Material::Handle& material)
		{
			this->material = material;
			return *this;
		}

		MaterialUnion& operator=(const MaterialUnion& other) = default;

		const Material& operator->() const { return *Get(); }
		const Material& operator*() const { return *Get(); }

		operator bool() const { return (bool)Get(); }

		const Common::Handle<Material>& Get() const
		{
			if (material.index() == 0)
				return std::get<0>(material)->Get();
			else
				return std::get<1>(material);
		}
	};

	class MaterialResourceList : public std::vector<Resources::MaterialResource::Handle>//, public Common::Resource
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