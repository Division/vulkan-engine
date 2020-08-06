#include "MaterialManager.h"
#include "Material.h"

namespace render {

	MaterialManager::~MaterialManager() = default;
	MaterialManager::MaterialManager() = default;

	uint32_t MaterialManager::GetMaterialID(const Material& material) const
	{
		std::lock_guard<std::mutex> lock(mutex);

		auto hash = material.GetHash();

		auto it = material_map.find(hash);
		if (it == material_map.end())
			material_map.insert(std::make_pair(hash, std::make_unique<Material>(material)));

		return hash;
	}

	const Material* MaterialManager::GetMaterial(uint32_t id) const
	{
		std::lock_guard<std::mutex> lock(mutex);

		auto it = material_map.find(id);
		return it != material_map.end() ? it->second.get() : nullptr;
	}
	
}