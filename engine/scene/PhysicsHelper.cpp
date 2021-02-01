#include "PhysicsHelper.h"
#include "Engine.h"
#include "Scene.h"
#include "ecs/components/Physics.h"

using namespace physx;
using namespace ECS;

namespace Physics::Helper
{
	PxRigidActor* AddPhysics(EntityManager& manager, EntityID entity, const PhysicsInitializer& init)
	{
		PxRigidActor* result = nullptr;
		auto* physics = Engine::Get()->GetScene()->GetPhysics();
		vec3 scale(init.size);

		if (init.is_static)
		{
			auto component = manager.AddComponent<components::RigidbodyStatic>(entity);
			switch (init.shape)
			{
			case PhysicsInitializer::Shape::Plane:
			{
				component->body = physics->CreatePlaneStatic(init.position, init.rotation, init.material);
				break;
			}
			
			case PhysicsInitializer::Shape::Sphere:
			{
				component->body = physics->CreateSphereStatic(init.position, init.rotation, init.size, init.material);
				break;
			}

			default:
				throw std::runtime_error("unsupported physics parameters");
			}

			result = component->body.get();
		}
		else
		{
			auto component = manager.AddComponent<components::RigidbodyDynamic>(entity);
			switch (init.shape)
			{
			case PhysicsInitializer::Shape::Box:
			{
				component->body = physics->CreateBoxDynamic(init.position, init.rotation, init.size, init.material);
				break;
			}

			case PhysicsInitializer::Shape::Sphere:
			{
				component->body = physics->CreateSphereDynamic(init.position, init.rotation, init.size, init.material);
				break;
			}

			default:
				throw std::runtime_error("unsupported physics parameters");
			}

			result = component->body.get();
		}

		static_assert(sizeof(result->userData) == sizeof(entity));
		result->userData = (void*)entity;
		return result;
	}
}