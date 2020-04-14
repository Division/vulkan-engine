#include "RendererSystem.h"
#include "ecs/components/MeshRenderer.h"
#include "ecs/components/Transform.h"
#include "ecs/components/CullingData.h"
#include "Engine.h"
#include "render/material/MaterialManager.h"
#include "render/material/Material.h"

namespace core { namespace ECS { namespace systems {

	using namespace core::ECS::components;

	void UpdateRendererSystem::Process(Chunk* chunk)
	{
		//auto& layout = chunk->GetComponentLayout();
		ProcessMeshRenderer(chunk);
	}

	void UpdateRendererSystem::ProcessMeshRenderer(Chunk* chunk)
	{
		OPTICK_EVENT();

		ComponentFetcher<MeshRenderer> mesh_renderer_fetcher(*chunk);
		ComponentFetcher<Transform> transform_fetcher(*chunk);
		ComponentFetcher<CullingData> culling_data_fetcher(*chunk);
		auto* material_manager = Engine::Get()->GetMaterialManager();

		for (int i = 0; i < chunk->GetEntityCount(); i++)
		{
			auto* mesh_renderer = mesh_renderer_fetcher.GetComponent(i);
			auto* transform = transform_fetcher.GetComponent(i);
			auto* material = material_manager->GetMaterial(mesh_renderer->material_id);
			mesh_renderer->object_params.transform = transform->local_to_world;
			mesh_renderer->object_params.uvOffset = vec2(0, 0);
			mesh_renderer->object_params.uvScale = vec2(1, 1);
			mesh_renderer->object_params.roughness = material->GetRoughness();

			if (culling_data_fetcher.HasData())
			{
				auto* culling_data = culling_data_fetcher.GetComponent(i);
				// Setting worldspace position for sphere type culling
				if (culling_data->type == CullingData::Type::Sphere)
					culling_data->sphere.worldspace.position = culling_data->sphere.local.position + vec3(transform->local_to_world[3]);
			}
		}
	}

} } }