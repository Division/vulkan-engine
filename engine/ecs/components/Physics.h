#include "scene/Physics.h"

namespace ECS::components
{
	struct RigidbodyDynamic
	{
		Physics::Handle<physx::PxRigidDynamic> body;
	};

	struct RigidbodyStatic
	{
		Physics::Handle<physx::PxRigidStatic> body;
	};

	// Implement to move static bodies
	/*struct RigidbodyKinematic
	{
		Physics::Handle<physx::PxRigidStatic> body;
	};*/
}