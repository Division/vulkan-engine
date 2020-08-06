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
		EntityAddress address;
		EntityID id = 0;
		const ComponentLayout* layout;
	};

}