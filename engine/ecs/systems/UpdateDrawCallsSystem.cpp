#include "UpdateDrawCallsSystem.h"
#include "ecs/components/MeshRenderer.h"
#include "ecs/components/MultiMeshRenderer.h"
#include "ecs/components/Transform.h"
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
		ComponentFetcher<MeshRenderer> mesh_renderer_fetcher(*chunk);
		ComponentFetcher<MultiMeshRenderer> multimesh_renderer_fetcher(*chunk);
		ComponentFetcher<Transform> transform_fetcher(*chunk);
		auto& layout = chunk->GetComponentLayout();
		const bool has_mesh_renderer = layout.GetComponentData(GetComponentHash<MeshRenderer>());
		const bool has_multimesh_renderer = layout.GetComponentData(GetComponentHash<MultiMeshRenderer>());

		if (has_mesh_renderer)
		{
			for (int i = 0; i < chunk->GetEntityCount(); i++)
			{
				auto* mesh_renderer = mesh_renderer_fetcher.GetComponent(i);
				if (mesh_renderer->draw_call_handle)
					continue;

				auto* transform = transform_fetcher.GetComponent(i);
				auto& material = mesh_renderer->material;
				auto handle = draw_call_manager.CreateHandle();
				auto draw_call = handle.AddDrawCall(*mesh_renderer->mesh, *material);
				mesh_renderer->draw_call_handle = std::move(handle);
			
				draw_call->transform = transform;
				draw_call->queue = mesh_renderer->render_queue;
				draw_call->object_params = &mesh_renderer->object_params; // TODO: move to AddDrawCall
			}
		}
		
		if (has_multimesh_renderer)
		{
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

				auto get_material_function = mesh_renderer->material_resources ? &GetMaterialResource : &GetMaterial;

				for (int j = 0; j < mesh_renderer->multi_mesh->GetMeshCount(); j++)
				{
					auto& mesh = mesh_renderer->multi_mesh->GetMesh(j);
					auto material = mesh_renderer->GetMaterial((size_t)j);
					auto draw_call = handle.AddDrawCall(*mesh, *material);
					draw_call->transform = transform;
					draw_call->queue = mesh_renderer->render_queue;
					draw_call->object_params = &mesh_renderer->object_params[j]; // TODO: move to AddDrawCall
				}

				mesh_renderer->draw_calls = std::move(handle);
			}
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