#pragma once

#include "ecs/System.h"

namespace ECS::systems
{
	class PhysicsPostUpdateSystem : public System
	{
	public:
		PhysicsPostUpdateSystem(EntityManager& manager) : System(manager) {}
		void Process(Chunk* chunk) override;
	};

	class PhysicsCharacterControllerSystem : public System
	{
	public:
		PhysicsCharacterControllerSystem(EntityManager& manager) : System(manager) {}
		void Process(Chunk* chunk) override;
	};

	class VehicleControlSystem : public System
	{
	public:
		VehicleControlSystem(EntityManager& manager) : System(manager) {}
		void Process(Chunk* chunk) override;
	};
}