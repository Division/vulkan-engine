#include "CommonIncludes.h"
#include "PhysicsSystem.h"
#include "ecs/components/Transform.h"
#include "ecs/components/Physics.h"
#include "ecs/components/Static.h"

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

	struct QueryCallback : public physx::PxQueryFilterCallback
	{
		physx::PxQueryHitType::Enum preFilter(const physx::PxFilterData& filterData, const physx::PxShape* shape, const physx::PxRigidActor* actor, physx::PxHitFlags& queryFlags)
		{
			return physx::PxQueryHitType::Enum::eBLOCK;
		}

		virtual physx::PxQueryHitType::Enum postFilter(const physx::PxFilterData& filterData, const physx::PxQueryHit& hit)
		{
			return physx::PxQueryHitType::Enum::eBLOCK;
		}
	};

	void PhysicsCharacterControllerSystem::Process(Chunk* chunk)
	{
		OPTICK_EVENT();
		ComponentFetcher<Transform> transform_fetcher(*chunk);
		ComponentFetcher<PhysXCharacterController> character_controller_fetcher(*chunk);
		auto dt = manager.GetStaticComponent<components::DeltaTime>();

		using namespace physx;

		QueryCallback cb;

		for (int i = 0; i < chunk->GetEntityCount(); i++)
		{
			auto* transform = transform_fetcher.GetComponent(i);
			auto* character_controller = character_controller_fetcher.GetComponent(i);

			if (!character_controller->controller)
				continue;

			const physx::PxExtendedVec3 current_pos(transform->position.x, transform->position.y, transform->position.z);
			const auto delta = current_pos - character_controller->controller->getFootPosition();
			
			physx::PxControllerFilters filters;
			auto collision_flags = character_controller->controller->move(delta, 0.0f, dt->dt, filters);

			const auto new_position = character_controller->controller->getFootPosition();
			transform->position = vec3(new_position.x, new_position.y, new_position.z);
		}
	}

}