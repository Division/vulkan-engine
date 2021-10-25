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
			
			if (!mesh_renderer->GetMultiMesh())
				continue;

			assert(mesh_renderer->HasMaterials());

			if (!mesh_renderer->HasMaterials())
			{
				continue;
			}

			assert(!mesh_renderer->draw_calls || mesh_renderer->draw_calls.GetDrawCallCount() == mesh_renderer->GetMultiMesh()->GetMeshCount());
			for (int j = 0; j < mesh_renderer->GetMultiMesh()->GetMeshCount(); j++)
			{
				if (mesh_renderer->draw_calls)
				{
					auto* draw_call = mesh_renderer->draw_calls.GetDrawCall(j);
					draw_call->obb = transform->GetOBB();
					draw_call->transform = transform->GetLocalToWorld();

					const bool is_uniform_scale = (std::abs(transform->scale.x - transform->scale.y) < 1e-4f) && (std::abs(transform->scale.x - transform->scale.z) < 1e-4f);

					if (is_uniform_scale)
						draw_call->normal_transform = mat3(draw_call->transform);
					else
						draw_call->normal_transform = glm::transpose(glm::inverse(mat3(draw_call->transform)));
				}
			}
		}
	}

} }