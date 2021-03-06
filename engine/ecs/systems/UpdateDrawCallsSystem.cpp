#include "UpdateDrawCallsSystem.h"
#include "ecs/components/MultiMeshRenderer.h"
#include "ecs/components/Transform.h"
#include "ecs/components/Entity.h"
#include "ecs/components/AnimationController.h"
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
		
		const bool has_skinning = chunk->GetComponentLayout().GetComponentData(ECS::GetComponentHash<components::AnimationController>());

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

				render::DrawCallInitializer initializer(*mesh, *material);
				initializer.SetHasSkinning(has_skinning);

				auto draw_call = handle.AddDrawCall(initializer);
					
				draw_call->queue = material->GetRenderQueue();
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
		auto* buffer = scene_buffers.GetObjectParamsBuffer();

		for (int i = 0; i < chunk->GetEntityCount(); i++)
		{
			auto* draw_call = draw_call_fetcher.GetComponent(i);
			const auto offset = buffer->Append(draw_call->object_params);
			draw_call->dynamic_offset = offset;
		}
	}

	void UploadSkinningSystem::Process(Chunk* chunk)
	{
		OPTICK_EVENT();
		ComponentFetcher<DrawCall> draw_call_fetcher(*chunk);
		ComponentFetcher<SkinningData> skinning_data_fetcher(*chunk);
		auto* buffer = scene_buffers.GetSkinningMatricesBuffer();

		for (int i = 0; i < chunk->GetEntityCount(); i++)
		{
			auto* draw_call = draw_call_fetcher.GetComponent(i);
			auto* skinning_data = skinning_data_fetcher.GetComponent(i);
			const auto offset = buffer->Append(skinning_data->bone_matrices);
			draw_call->skinning_dynamic_offset = offset;
		}
	}

} }