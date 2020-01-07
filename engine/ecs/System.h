#pragma once

#include "ECS.h"
#include "EntityChunks.h"

namespace core { namespace ECS {

	class System
	{
	public:
		System(EntityManager& manager)
			: manager(manager) {}

		virtual void Process(const ChunkList::List& list) = 0;

	protected:
		EntityManager& manager;
	};

} }