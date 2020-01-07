#pragma once 

#include "ecs/System.h"

namespace core { namespace ECS { namespace systems {

	class TransformSystem : public System
	{
	public:
		TransformSystem(EntityManager& manager) : System(manager) {}

		void Process(const ChunkList::List& list) override;
	};

} } }