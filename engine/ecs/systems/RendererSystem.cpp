#include "RendererSystem.h"
#include "ecs/components/MeshRenderer.h"
#include "ecs/components/MultiMeshRenderer.h"
#include "ecs/components/Transform.h"
#include "Engine.h"
#include "render/material/Material.h"

namespace ECS { namespace systems {

	using namespace ECS::components;

	void UpdateRendererSystem::Process(Chunk* chunk)
	{
		const auto& layout = chunk->GetComponentLayout();
		const bool has_mesh_renderer = layout.GetComponentData(GetComponentHash<MeshRenderer>());
		const bool has_multimesh_renderer = layout.GetComponentData(GetComponentHash<MultiMeshRenderer>());
		
		if (has_mesh_renderer)
			ProcessMeshRenderer(chunk);
		if (has_multimesh_renderer)
			ProcessMultiMeshRenderer(chunk);
	}

	void UpdateRendererSystem::ProcessMeshRenderer(Chunk* chunk)
	{
		OPTICK_EVENT();

		ComponentFetcher<MeshRenderer> mesh_renderer_fetcher(*chunk);
		ComponentFetcher<Transform> transform_fetcher(*chunk);

		for (int i = 0; i < chunk->GetEntityCount(); i++)
		{
			auto* mesh_renderer = mesh_renderer_fetcher.GetComponent(i);
			auto* transform = transform_fetcher.GetComponent(i);
			auto& material = mesh_renderer->material;
			mesh_renderer->object_params.transform = transform->local_to_world;
			mesh_renderer->object_params.uvOffset = vec2(0, 0);
			mesh_renderer->object_params.uvScale = vec2(1, 1);
			mesh_renderer->object_params.roughness = material->GetRoughness();
			mesh_renderer->object_params.metalness = material->GetMetalness();
			mesh_renderer->object_params.color = material->GetColor();
		}
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
			mesh_renderer->object_params.resize(mesh_renderer->multi_mesh->GetMeshCount());

			assert(mesh_renderer->materials->size() > 0);

			if (mesh_renderer->materials->empty())
			{
				mesh_renderer->object_params.clear();
				continue;
			}

			for (int j = 0; j < mesh_renderer->multi_mesh->GetMeshCount(); j++)
			{
				auto& object_params = mesh_renderer->object_params[j];
				auto& material = (*mesh_renderer->materials)[std::min((size_t)j, mesh_renderer->materials->size() - 1)];
				object_params.transform = transform->local_to_world;
				object_params.uvOffset = vec2(0, 0);
				object_params.uvScale = vec2(1, 1);
				object_params.roughness = material->GetRoughness();
				object_params.metalness = material->GetMetalness();
				object_params.color = material->GetColor();
			}
		}
	}

} }