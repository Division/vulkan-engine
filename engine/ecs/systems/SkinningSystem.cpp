#include "SkinningSystem.h"
#include "Engine.h"
#include "ecs/components/AnimationController.h"
#include "ecs/components/Static.h"
#include "ecs/components/Transform.h"
#include "render/debug/DebugDraw.h"
#include "ozz/animation/runtime/skeleton_utils.h"

namespace ECS::systems {

	void SkinningSystem::Process(Chunk* chunk)
	{
		ComponentFetcher<components::AnimationController> animation_controller_fetcher(*chunk);
		auto dt = manager.GetStaticComponent<components::DeltaTime>();

		for (int i = 0; i < chunk->GetEntityCount(); i++)
		{
			auto* animation_controller = animation_controller_fetcher.GetComponent(i);
			animation_controller->mixer->Update(dt->dt);
			animation_controller->mixer->ProcessBlending();
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
			for (int i = 0; i < model_matrices.size(); i++)
			{
				glm::mat4x4& m = (glm::mat4x4&)model_matrices[i];
				vec4 p = m * vec4(transform->WorldPosition(), 1);
				Engine::Get()->GetDebugDraw()->DrawPoint(glm::vec3(p), vec3(1, 1, 1), 8);

				const auto parent_id = skeleton->Get()->joint_parents()[i];
				if (parent_id != ozz::animation::Skeleton::kNoParent)
				{
					glm::mat4x4& m = (glm::mat4x4&)model_matrices[parent_id];
					vec4 parent = m * vec4(transform->WorldPosition(), 1);
					Engine::Get()->GetDebugDraw()->DrawLine(glm::vec3(p), vec3(parent), vec4(1, 1, 1, 1));
				}
			}
		}
	}

}