#include "TransformSystem.h"

namespace core { namespace ECS { namespace systems {

	void TransformSystem::Process(const ChunkList::List& list)
	{
		for (auto* chunk_list : list)
		{
			Chunk* chunk = chunk_list->GetFirstChunk();
			auto& layout = chunk_list->GetLayout();
			 layout.GetComponentData
			while (chunk)
			{
				chunk->
			}
		}
	}

} } }