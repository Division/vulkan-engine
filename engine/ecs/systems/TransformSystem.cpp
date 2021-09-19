#include "CommonIncludes.h"
#include "TransformSystem.h"
#include "ecs/components/Transform.h"

namespace ECS { namespace systems {

	using namespace components;

	void RootTransformSystem::Process(Chunk* chunk)
	{
		OPTICK_EVENT();

		ComponentFetcher<RootTransform> root_transform_fetcher(*chunk);
		ComponentFetcher<Transform> transform_fetcher(*chunk);

		for (int i = 0; i < chunk->GetEntityCount(); i++)
		{
			auto* root_transform = root_transform_fetcher.GetComponent(i);
			auto* parent_transform = transform_fetcher.GetComponent(i);
			auto& children = transform_graph.GetChildren(root_transform->id);
			
			SetTransformRecursive(parent_transform->GetLocalToWorld(), children);
		}
	}

	void RootTransformSystem::SetTransformRecursive(const mat4& matrix, const std::vector<EntityID>& children)
	{
		for (auto child_id : children)
		{
			auto* child_transform = manager.GetComponent<Transform>(child_id);
			auto child_matrix = ComposeMatrix(child_transform->position, child_transform->rotation, child_transform->scale);
			child_transform->SetLocalToWorld(matrix * child_matrix);
			SetTransformRecursive(child_transform->GetLocalToWorld(), transform_graph.GetChildren(child_id));
		}
	}

	void NoChildTransformSystem::Process(Chunk* chunk)
	{
		OPTICK_EVENT();
		ComponentFetcher<Transform> transform_fetcher(*chunk);

		for (int i = 0; i < chunk->GetEntityCount(); i++)
		{
			auto* transform = transform_fetcher.GetComponent(i);
			transform->SetLocalToWorld(ComposeMatrix(transform->position, transform->rotation, transform->scale));
		}
	}

} }