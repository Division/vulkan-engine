#pragma once

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

	struct Vehicle
	{
		physx::PxVehicleDrive4W* vehicle = nullptr;
		physx::PxVehicleDrive4WRawInputData input;

		Vehicle() = default;
		Vehicle(Vehicle&& other)
		{
			if (vehicle)
			{
				vehicle->free();
			}

			input = std::move(other.input);
			vehicle = other.vehicle;
			other.vehicle = nullptr;
		}

		~Vehicle()
		{
			if (vehicle)
			{
				vehicle->free();
				vehicle = nullptr;
			}
		}
	};
}