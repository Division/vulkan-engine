#pragma once

#include "render/renderer/IRenderer.h"
#include "render/shader/ShaderBufferStruct.h"
#include "resources/MultiMesh.h"
#include "resources/MaterialResource.h"
#include "render/material/Material.h"
#include "utils/DataStructures.h"
#include "render/renderer/DrawCallManager.h"
#include "utils/Math.h"
#include "Transform.h"

class Mesh;
class Material;

namespace ECS::components 
{

	struct MultiMeshRenderer
	{
	private:
		Resources::MultiMesh::Handle multi_mesh;
		Common::Handle<Resources::MultiMesh> common_multi_mesh;
	
	public:
		render::MaterialList::Handle materials;
		render::MaterialResourceList::Handle material_resources;
		render::DrawCallManager::Handle draw_calls;
		uint32_t instance_count = 1; // Number of instanced per draw call

		bool HasMaterials()
		{
			return (materials && materials->size()) || (material_resources && material_resources->size());
		}

		void SetMultiMesh(Resources::MultiMesh::Handle value)
		{
			multi_mesh = value;
			common_multi_mesh = nullptr;
		}

		void SetMultiMesh(Common::Handle<Resources::MultiMesh> value)
		{
			common_multi_mesh = value;
			multi_mesh = nullptr;
		}

		const Resources::MultiMesh* GetMultiMesh() const
		{
			if (multi_mesh)
				return &*multi_mesh;

			if (common_multi_mesh)
				return &*common_multi_mesh;

			return nullptr;
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
		}

		static void Initialize(EntityManager& manager, EntityID id, MultiMeshRenderer* multi_mesh)
		{
			auto transform = manager.GetComponent<Transform>(id);
			if (transform)
			{
				transform->bounds = multi_mesh->GetMultiMesh()->GetBounds();
			}
		}

	};

}