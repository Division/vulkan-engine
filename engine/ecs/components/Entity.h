#pragma once

namespace core { namespace ECS {

	class Chunk;

	struct EntityAddress
	{
		Chunk* chunk = nullptr;
		uint32_t index = 0;
	};

	typedef uint64_t EntityID;

	struct EntityData
	{
		EntityAddress address;
		uint32_t type_set_hash = 0;
		uint32_t component_count = 0;
		EntityID id = 0;
	};

} }