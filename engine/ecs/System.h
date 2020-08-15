#pragma once

#include "ECS.h"
#include "EntityChunks.h"

namespace ECS {

	class System
	{
	public:
		System(EntityManager& manager, bool multithreaded = true)
			: manager(manager)
			, supports_multithreading(multithreaded) 
		{}

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
		bool supports_multithreading;
		EntityManager& manager;
	};

	class CallbackSystem : public System
	{
	public:
		CallbackSystem(std::function<void(Chunk*)> process_callback, EntityManager& manager, bool multithreaded = true)
			: System(manager, multithreaded), process_callback(process_callback) 
		{}

		virtual void Process(Chunk* chunk) override
		{
			process_callback(chunk);
		}

	private:
		std::function<void(Chunk*)> process_callback;

	};

}