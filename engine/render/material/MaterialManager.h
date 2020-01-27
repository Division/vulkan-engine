#pragma once

#include <mutex>
#include "CommonIncludes.h"

class Material;

namespace core { namespace render {

	class MaterialManager
	{
	public:
		MaterialManager();
		~MaterialManager();
		uint32_t GetMaterialID(const Material& material) const;
		const Material* GetMaterial(uint32_t id) const;

	private:
		mutable std::unordered_map<uint32_t, std::unique_ptr<const Material>> material_map;
		mutable std::mutex mutex;
	};

} }