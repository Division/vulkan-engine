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
}