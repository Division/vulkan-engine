#include "SkinningSystem.h"
#include "Engine.h"
#include "ecs/components/AnimationController.h"
#include "ecs/components/Static.h"
#include "ecs/components/Transform.h"
#include "ecs/components/DrawCall.h"
#include "ecs/components/MultiMeshRenderer.h"
#include "render/debug/DebugDraw.h"
#include "ozz/animation/runtime/skeleton_utils.h"

namespace ECS::systems {

	void SkinningSystem::Process(Chunk* chunk)
	{
		ComponentFetcher<components::AnimationController> animation_controller_fetcher(*chunk);
		ComponentFetcher<components::MultiMeshRenderer> multi_mesh_fetcher(*chunk);
		ComponentFetcher<components::Transform> transform_fetcher(*chunk);

		auto dt = manager.GetStaticComponent<components::DeltaTime>();

		for (int i = 0; i < chunk->GetEntityCount(); i++)
		{
			// Calculate bones model transformation
			auto* animation_controller = animation_controller_fetcher.GetComponent(i);
			animation_controller->mixer->Update(dt->dt);
			animation_controller->mixer->ProcessBlending();

			// Write transformation to the skinning data, multiplied by inverse bindpose and world transform
			auto* multi_mesh = multi_mesh_fetcher.GetComponent(i);
			auto* transform = transform_fetcher.GetComponent(i);

			auto model_matrices = animation_controller->mixer->GetModelMatrices();

			auto bounds = multi_mesh->multi_mesh->GetMesh(0)->aabb();

			for (int j = 0; j < multi_mesh->multi_mesh->GetMeshCount(); j++)
			{
				if (!multi_mesh->draw_calls) continue;
				

				auto* skinning_data = multi_mesh->draw_calls.GetSkinningData(j);
				if (!skinning_data) continue;

				auto& mesh = multi_mesh->multi_mesh->GetMesh(j);
				auto& inv_bind_pose = multi_mesh->multi_mesh->GetInvBindPose(j);
				const AABB mesh_bounds = mesh->aabb();
				bounds.expand(mesh_bounds.min);
				bounds.expand(mesh_bounds.max);

				for (uint16_t k = 0; k < mesh->GetBoneCount(); k++)
				{
					const auto remap_index = mesh->GetBoneRemapIndex(k);
					skinning_data->bone_matrices.matrices[k] = transform->local_to_world * (mat4&)model_matrices[remap_index] * inv_bind_pose[k];
				}
			}

			//auto b1 = vec3((mat4&)model_matrices[0] * vec4(bounds.min, 1));
			//auto b2 = vec3((mat4&)model_matrices[0] * vec4(bounds.max, 1));
			auto b1 = bounds.min;
			auto b2 = bounds.max;
			transform->bounds = AABB(glm::min(b1, b2), glm::max(b1, b2));
		}
	}

	
	void DebugDrawSkinningSystem::Process(Chunk* chunk)
	{
		ComponentFetcher<components::Transform> transform_fetcher(*chunk);
		ComponentFetcher<components::AnimationController> animation_controller_fetcher(*chunk);

		for (int i = 0; i < chunk->GetEntityCount(); i++)
		{
			auto* transform = transform_fetcher.GetComponent(i);
			auto* animation_controller = animation_controller_fetcher.GetComponent(i);

			auto skeleton = animation_controller->mixer->GetSkeleton();
			auto model_matrices = animation_controller->mixer->GetModelMatrices();
			//auto& model_matrices = skeleton->GetBindposeMatrices();
			for (int i = 0; i < model_matrices.size(); i++)
			{
				glm::mat4x4& m = (glm::mat4x4&)model_matrices[i];
				vec4 p = transform->local_to_world * m * vec4(0, 0, 0, 1);
				Engine::Get()->GetDebugDraw()->DrawPoint(glm::vec3(p), vec3(0.2, 0.2, 1), 8);

				const auto parent_id = skeleton->Get()->joint_parents()[i];
				if (parent_id != ozz::animation::Skeleton::kNoParent)
				{
					glm::mat4x4& m = (glm::mat4x4&)model_matrices[parent_id];
					vec4 parent = transform->local_to_world * m * vec4(0, 0, 0, 1);
					Engine::Get()->GetDebugDraw()->DrawLine(glm::vec3(p), vec3(parent), vec4(0, 0, 1, 1));
				}
			}
		}
	}

	void DebugDrawSkinningVerticesSystem::Process(Chunk* chunk)
	{
		ComponentFetcher<components::Transform> transform_fetcher(*chunk);
		ComponentFetcher<components::AnimationController> animation_controller_fetcher(*chunk);
		ComponentFetcher<components::MultiMeshRenderer> multi_mesh_renderer_fetcher(*chunk);

		for (int i = 0; i < chunk->GetEntityCount(); i++)
		{
			auto* transform = transform_fetcher.GetComponent(i);
			auto* animation_controller = animation_controller_fetcher.GetComponent(i);
			auto* multi_mesh_renderer = multi_mesh_renderer_fetcher.GetComponent(i);

			auto* skeleton = animation_controller->mixer->GetSkeleton()->Get();
			auto model_matrices = animation_controller->mixer->GetModelMatrices();
			//auto& model_matrices = animation_controller->mixer->GetSkeleton()->GetBindposeMatrices();

			//auto& inv_bind_poses = animation_controller->mixer->GetSkeleton()->GetInverseBindposeMatrices();

			auto mesh_count = multi_mesh_renderer->multi_mesh->GetMeshCount();
			for (int k = 0; k < mesh_count; k++)
			{
				auto& mesh = multi_mesh_renderer->multi_mesh->GetMesh(k);
				auto& inv_bind_poses = multi_mesh_renderer->multi_mesh->GetInvBindPose(k);

				for (int v = 0; v < mesh->vertexCount(); v++)
				{
					auto result = vec3(0, 0, 0);
					auto p = mesh->getVertex(v);
					auto indexes = mesh->GetSkeletonMappedJointIndices(v);
					auto unmapped_indexes = mesh->getJointIndices(v);
					auto weights = mesh->getWeights(v);

					for (int j = 0; j < 4; j++)
					{
						glm::mat4x4& m = (glm::mat4x4&)model_matrices[indexes[j]];
						auto ibp = inv_bind_poses[unmapped_indexes.data[j]];
						auto pos = vec4(p, 1);
						result += vec3(transform->local_to_world * m * ibp * pos) * weights[j];
					}

					Engine::Get()->GetDebugDraw()->DrawPoint(glm::vec3(result), vec3(1, 1, 1), 4);
				}
			}
		}
	} 

}