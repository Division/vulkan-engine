#include "RendererSystem.h"
#include "ecs/components/MultiMeshRenderer.h"
#include "ecs/components/Transform.h"
#include "ecs/components/DrawCall.h"
#include "Engine.h"
#include "render/material/Material.h"

namespace ECS { namespace systems {

	using namespace ECS::components;

	void UpdateRendererSystem::Process(Chunk* chunk)
	{
		ProcessMultiMeshRenderer(chunk);
	}

	void UpdateRendererSystem::ProcessMultiMeshRenderer(Chunk* chunk)
	{
		OPTICK_EVENT();

		ComponentFetcher<MultiMeshRenderer> mesh_renderer_fetcher(*chunk);
		ComponentFetcher<Transform> transform_fetcher(*chunk);

		for (int i = 0; i < chunk->GetEntityCount(); i++)
		{
			auto* mesh_renderer = mesh_renderer_fetcher.GetComponent(i);
			auto* transform = transform_fetcher.GetComponent(i);
			
			if (mesh_renderer->object_params.size() < mesh_renderer->multi_mesh->GetMeshCount())
			{
				assert(!mesh_renderer->draw_calls);
				mesh_renderer->object_params.resize(mesh_renderer->multi_mesh->GetMeshCount());
			}

			assert(mesh_renderer->HasMaterials());

			if (!mesh_renderer->HasMaterials())
			{
				mesh_renderer->object_params.clear();
				continue;
			}

			assert(!mesh_renderer->draw_calls || mesh_renderer->draw_calls.GetDrawCallCount() == mesh_renderer->multi_mesh->GetMeshCount());
			for (int j = 0; j < mesh_renderer->multi_mesh->GetMeshCount(); j++)
			{
				auto& object_params = mesh_renderer->object_params[j];
				auto& material = mesh_renderer->GetMaterial(j);
				object_params.transform = transform->local_to_world;
				object_params.uvOffset = vec2(0, 0);
				object_params.uvScale = vec2(1, 1);
				object_params.roughness = material->GetRoughness();
				object_params.metalness = material->GetMetalness();
				object_params.color = material->GetColor();
				
				if (mesh_renderer->draw_calls)
				{
					auto* draw_call = mesh_renderer->draw_calls.GetDrawCall(j);
					draw_call->obb = transform->GetOBB();
					draw_call->object_params = object_params;
				}
			}
		}
	}

} }