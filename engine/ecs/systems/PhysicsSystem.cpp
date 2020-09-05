#include "CommonIncludes.h"
#include "PhysicsSystem.h"
#include "ecs/components/Transform.h"
#include "ecs/components/Physics.h"

using namespace ECS::components;

namespace ECS::systems
{

	// Write simulated data into transform
	void PhysicsPostUpdateSystem::Process(Chunk* chunk)
	{
		OPTICK_EVENT();
		ComponentFetcher<Transform> transform_fetcher(*chunk);
		ComponentFetcher<RigidbodyDynamic> rigidbody_fetcher(*chunk);

		for (int i = 0; i < chunk->GetEntityCount(); i++)
		{
			auto* transform = transform_fetcher.GetComponent(i);
			auto* rigidbory = rigidbody_fetcher.GetComponent(i);

			auto pose = rigidbory->body->getGlobalPose();
			Physics::ConvertTransform(pose, transform->position, transform->rotation);
		}
	}

}