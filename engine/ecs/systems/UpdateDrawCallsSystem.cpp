#include "UpdateDrawCallsSystem.h"
#include "ecs/components/MultiMeshRenderer.h"
#include "ecs/components/Transform.h"
#include "ecs/components/Entity.h"
#include "render/renderer/SceneBuffers.h"
#include "Engine.h"

namespace ECS { namespace systems {

	using namespace ECS::components;

	const Material::Handle& GetMaterial(MultiMeshRenderer* mesh_renderer, size_t index)
	{
		return (*mesh_renderer->materials)[std::min(index, mesh_renderer->materials->size() - 1)];
	};

	const Material::Handle& GetMaterialResource(MultiMeshRenderer* mesh_renderer, size_t index)
	{
		return (*mesh_renderer->material_resources)[std::min(index, mesh_renderer->material_resources->size() - 1)]->Get();
	};

	void CreateDrawCallsSystem::Process(Chunk* chunk)
	{
		OPTICK_EVENT();
		ComponentFetcher<MultiMeshRenderer> multimesh_renderer_fetcher(*chunk);
		ComponentFetcher<Transform> transform_fetcher(*chunk);
		ComponentFetcher<EntityData> entity_fetcher(*chunk);
		
		for (int i = 0; i < chunk->GetEntityCount(); i++)
		{
			auto* mesh_renderer = multimesh_renderer_fetcher.GetComponent(i);
			if (mesh_renderer->draw_calls)
				continue;

			if (mesh_renderer->object_params.size() == 0)
				continue;

			if (mesh_renderer->object_params.size() != mesh_renderer->multi_mesh->GetMeshCount())
				continue;

			assert((bool)mesh_renderer->material_resources ^ (bool)mesh_renderer->materials);

			auto handle = draw_call_manager.CreateHandle();
			auto* transform = transform_fetcher.GetComponent(i);
			auto obb = transform->GetOBB();

			auto get_material_function = mesh_renderer->material_resources ? &GetMaterialResource : &GetMaterial;

			for (int j = 0; j < mesh_renderer->multi_mesh->GetMeshCount(); j++)
			{
				auto& mesh = mesh_renderer->multi_mesh->GetMesh(j);
				auto material = mesh_renderer->GetMaterial((size_t)j);
				auto draw_call = handle.AddDrawCall(*mesh, *material);
				draw_call->queue = mesh_renderer->render_queue;
				draw_call->object_params = mesh_renderer->object_params[j]; // TODO: move to AddDrawCall
				draw_call->obb = obb;
			}

			mesh_renderer->draw_calls = std::move(handle);
		}
		
	}


	void UploadDrawCallsSystem::Process(Chunk* chunk)
	{
		OPTICK_EVENT();
		ComponentFetcher<DrawCall> draw_call_fetcher(*chunk);
		// TODO: add skinning
		auto* buffer = scene_buffers.GetObjectParamsBuffer();

		for (int i = 0; i < chunk->GetEntityCount(); i++)
		{
			auto* draw_call = draw_call_fetcher.GetComponent(i);
			auto offset = buffer->Append(draw_call->object_params);
			draw_call->dynamic_offset = offset;
		}
	}

} }