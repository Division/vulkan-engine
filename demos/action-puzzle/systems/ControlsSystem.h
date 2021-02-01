#pragma once

#include "ecs/System.h"

namespace ECS::systems
{
	class ControlsSystem : public ECS::System
	{
	public:
		ControlsSystem(ECS::EntityManager& manager) : ECS::System(manager) {}
		
		void Process(ECS::Chunk* chunk) override;
	};

	class VehicleControlSystem : public ECS::System
	{
	public:
		VehicleControlSystem(ECS::EntityManager& manager) : ECS::System(manager) {}

		void Process(ECS::Chunk* chunk) override;

	};

}