#pragma once

#include "render/renderer/IRenderer.h"
#include "render/shader/ShaderBufferStruct.h"
#include "resources/MultiMesh.h"
#include "resources/MaterialResource.h"
#include "render/material/Material.h"
#include "utils/DataStructures.h"
#include "render/renderer/DrawCallManager.h"
#include "utils/Math.h"

class Mesh;
class Material;

namespace ECS::components 
{

	struct MultiMeshRenderer
	{
		Resources::MultiMesh::Handle multi_mesh;
		render::MaterialList::Handle materials;
		render::MaterialResourceList::Handle material_resources;
		std::vector<Device::ShaderBufferStruct::ObjectParams> object_params;
		render::DrawCallManager::Handle draw_calls;

		bool HasMaterials()
		{
			return (materials && materials->size()) || (material_resources && material_resources->size());
		}

		const Material::Handle GetMaterial(size_t index) const
		{
			if (materials)
			{
				return (*materials)[std::min(index, materials->size() - 1)];
			}
			else
			{
				auto used_index = std::min(index, material_resources->size() - 1);
				return (*material_resources)[used_index]->Get();
			}
		};
	};

}