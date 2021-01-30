#pragma once

namespace ECS {

	class Chunk;
	class ComponentLayout;

	struct EntityAddress
	{
		Chunk* chunk = nullptr;
		uint32_t index = 0;
	};

	typedef uint64_t EntityID;

	struct EntityData
	{
		EntityID id = 0;
	};


	namespace components
	{
		struct RootEntity
		{
			//utils::SmallVector<EntityID, 4> entities;

			// TODO: implement destruction of related entities

			RootEntity()
			{

			}

			~RootEntity()
			{
			
			}
		};
	}

}