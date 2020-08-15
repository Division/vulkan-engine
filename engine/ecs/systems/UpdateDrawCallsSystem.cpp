#include "UpdateDrawCallsSystem.h"
#include "ecs/components/MeshRenderer.h"
#include "ecs/components/Transform.h"
#include "render/material/MaterialManager.h"
#include "render/renderer/SceneBuffers.h"
#include "Engine.h"

namespace ECS { namespace systems {

	using namespace ECS::components;

	void CreateDrawCallsSystem::Process(Chunk* chunk)
	{
		OPTICK_EVENT();
		ComponentFetcher<MeshRenderer> mesh_renderer_fetcher(*chunk);
		ComponentFetcher<Transform> transform_fetcher(*chunk);
		auto* material_manager = Engine::Get()->GetMaterialManager();

		for (int i = 0; i < chunk->GetEntityCount(); i++)
		{
			auto* mesh_renderer = mesh_renderer_fetcher.GetComponent(i);
			if (mesh_renderer->draw_call_id) // TODO: replace with component
				continue;

			auto* transform = transform_fetcher.GetComponent(i);
			auto* material = material_manager->GetMaterial(mesh_renderer->material_id);
			auto result = draw_call_manager.AddDrawCall(*mesh_renderer->mesh, *material);
			mesh_renderer->draw_call_id = result.first;
			
			result.second->transform = transform;
			result.second->queue = mesh_renderer->render_queue;
			result.second->object_params = &mesh_renderer->object_params; // TODO: move to AddDrawCall
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
			auto offset = buffer->Append(*draw_call->object_params);
			draw_call->dynamic_offset = offset;
		}
	}

} }