#include "render/mesh/Mesh.h"
#include "render/material/Material.h"

namespace ECS::components
{

	struct BatchingVolume
	{
		static uint32_t GetBatchHash(Material* material, Mesh* mesh)
		{
			const size_t material_ptr = (size_t)material;
			const std::pair<uint32_t, size_t> hashes = { mesh ? mesh->GetVertexLayout().GetHash() : 0u, material_ptr };
			return FastHash(&hashes, sizeof(hashes));
		}

		struct BatchSrc
		{
			vec3 position = vec3(0);
			vec3 scale = vec3(1);
			quat rotation = quat();
			Mesh::Handle mesh;
			Material::Handle material;

			uint32_t GetHash() const { return GetBatchHash(material.get(), mesh.get()); }
		};

		std::vector<BatchSrc> src_meshes;
		bool is_dirty = true;
	};

}