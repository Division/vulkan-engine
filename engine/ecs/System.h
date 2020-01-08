#pragma once

#include "ECS.h"
#include "EntityChunks.h"

namespace core { namespace ECS {

	class System
	{
	public:
		System(EntityManager& manager)
			: manager(manager) {}

		virtual void ProcessChunks(const ChunkList::List& list)
		{
			for (auto* chunk_list : list)
			{
				auto* chunk = chunk_list->GetFirstChunk();
				while (chunk)
				{
					Process(chunk);
					chunk = chunk->GetNextChunk();
				}
			}
		}

		virtual void Process(Chunk* chunk) = 0;

	protected:
		EntityManager& manager;
	};

} }