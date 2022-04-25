#include "LandMeshGenerator.h"
#include <ecs/components/Transform.h>
#include <ecs/components/MultiMeshRenderer.h>
#include <render/mesh/Mesh.h>
#include <loader/FileLoader.h>
#include "resources/Heightmap.h"

namespace Generation
{

	using namespace ECS;

	Mesh::Handle GenerateHeightmapMesh(vec3 scale, ivec2 resolution, float2 uv_repeat, gsl::span<const float> heightmap)
	{
		if (resolution.x * resolution.y > heightmap.size())
			throw std::runtime_error("data size is too small");

		auto mesh = Mesh::Create(false);

		std::vector<vec3> vertices;
		std::vector<vec2> tex_coords;
		std::vector<uint32_t> indices;

		ivec2 actualDimensions = resolution;
		vec3 cellShift = vec3(scale.x / (actualDimensions.x), 0, scale.z / (actualDimensions.y));

		for (int i = 0; i < actualDimensions.y; i++) {
			for (int j = 0; j < actualDimensions.x; j++) {
				int index = i * actualDimensions.x + j;
				float y = scale.y * heightmap[index];
				vec3 origin = vec3(j * cellShift.x, y, i * cellShift.z);
				vec2 uv((float)j * uv_repeat.x / (actualDimensions.x - 1.0f), -(float)i * uv_repeat.y / (actualDimensions.y - 1.0f));
				vertices.push_back(origin);
				tex_coords.push_back(uv);

				if (i < actualDimensions.y - 1 && j < actualDimensions.x - 1) {
					int index0 = index;
					int index1 = index0 + 1;
					int index2 = index0 + actualDimensions.x;
					int index3 = index0 + actualDimensions.x + 1;
					indices.push_back((uint32_t)index0);
					indices.push_back((uint32_t)index2);
					indices.push_back((uint32_t)index1);
					indices.push_back((uint32_t)index2);
					indices.push_back((uint32_t)index3);
					indices.push_back((uint32_t)index1);
				}
			}
		}

		mesh->setVertices(vertices);
		mesh->setTexCoord0(tex_coords);
		mesh->setIndices(indices);

		mesh->calculateNormals();
		mesh->calculateTBN();
		mesh->createBuffer();

		return mesh;
	}

	ECS::EntityID CreateLandMesh(ECS::EntityManager& manager, vec3 position, std::wstring_view heightmap_path)
	{
		auto entity = manager.CreateEntity();
		{
			auto transform = manager.AddComponent<components::Transform>(entity);
			transform->position = position;
		}

		Resources::Heightmap::Handle heightmap(heightmap_path);
		Common::Handle<Mesh> mesh = GenerateHeightmapMesh(vec3(128, 0.1, 128) * 20.0f, ivec2(256, 256), vec2(10, 10), heightmap->GetData());

		{
			auto renderer = manager.AddComponent<components::MultiMeshRenderer>(entity);
			auto multi_mesh = Resources::MultiMesh::Create({ &mesh, 1 }, mesh->aabb());
			renderer->SetMultiMesh(multi_mesh);
			renderer->materials = render::MaterialList::Create();

			Material::Handle material = Material::Create();
			material->LightingEnabled(true);
			renderer->materials->push_back(material);
		}
	}

}