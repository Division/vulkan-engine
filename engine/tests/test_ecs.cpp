#include "lib/catch/catch.hpp"
#include "ecs/ECS.h"

using namespace core::ECS;

struct TestPositionComponent
{
	vec3 position;
};

struct TestAABBComponent
{
	vec3 min;
	vec3 max;
};

TEST_CASE("ECS entity spawn/destroy") 
{
	EntityManager manager;

	auto entity = manager.CreateEntity();
	auto& address_map = manager.GetEntityAddressMap();
	auto& chunk_map = manager.GetChunkMap();

	auto* entity_component = manager.GetComponent<EntityData>(entity);
	REQUIRE(entity_component->id == entity);

	// Check address for entity
	auto entity_address_it = address_map.find(entity);
	REQUIRE(address_map.size() == 1);
	REQUIRE(entity_address_it != address_map.end());
	REQUIRE(chunk_map.size() == 1);
	REQUIRE(entity_address_it->second.chunk == entity_component->address.chunk);

	// Check EntityData component exists
	auto address = entity_address_it->second;
	auto* chunk = address.chunk;
	auto layout = address.chunk->GetComponentLayout();
	REQUIRE(layout.GetComponentCount() == 1);
	REQUIRE(layout.GetComponentData(GetComponentHash<EntityData>()) != nullptr);
	REQUIRE(layout.GetComponentData(GetComponentHash<EntityData>() + 1) == nullptr);
	REQUIRE(chunk->GetEntityCount() == 1);

	// Destroying entity
	manager.DestroyEntity(entity);
	REQUIRE(address_map.find(entity) == address_map.end());
	REQUIRE(chunk->GetEntityCount() == 0);
	REQUIRE(chunk_map.size() == 1); // chunk remains in place
}

TEST_CASE("ECS multiple entities and components")
{
	EntityManager manager;
	auto& address_map = manager.GetEntityAddressMap();
	auto& chunk_map = manager.GetChunkMap();

	std::array<EntityID, 10> entities;

	for (auto& entity : entities)
		entity = manager.CreateEntity();

	for (auto entity : entities)
	{
		auto* position = manager.AddComponent<TestPositionComponent>(entity);
		position->position = vec3(1, 0, 1);
	}
	
	auto address0 = address_map.at(entities[0]);

	for (int i = 0; i < entities.size() / 2; i++)
		manager.AddComponent<TestAABBComponent>(entities[i]);

	auto address1 = address_map.at(entities[0]);
	REQUIRE(address1.chunk != address0.chunk); // Chunk must be changed after component was added

	// Existing component value should remain the same after another component is added
	for (int i = 0; i < entities.size() / 2; i++)
	{
		auto* position = manager.GetComponent<TestPositionComponent>(entities[i]);
		REQUIRE(position->position == vec3(1, 0, 1));
	}
	
	REQUIRE(address_map.size() == entities.size());
	REQUIRE(chunk_map.size() == 3); // EntityData only, EntityData + TestPositionComponent, EntityData + TestPositionComponent + TestAABBComponent

	manager.RemoveComponent<TestPositionComponent>(entities[0]);
	REQUIRE(chunk_map.size() == 4); // More one combination added
	address0 = address_map.at(entities[0]);
	auto& layout = address0.chunk->GetComponentLayout();
	REQUIRE(layout.GetComponentCount() == 2);
	REQUIRE(layout.GetComponentData(GetComponentHash<TestPositionComponent>()) == nullptr);
	REQUIRE(layout.GetComponentData(GetComponentHash<TestAABBComponent>()) != nullptr);
}

TEST_CASE("ECS allocation of multiple chunks with the same layout")
{
	EntityManager manager;
	auto& address_map = manager.GetEntityAddressMap();
	auto& chunk_map = manager.GetChunkMap();

	SECTION("no component entity")
	{
		auto entity = manager.CreateEntity();
		auto* entity_data = manager.GetComponent<EntityData>(entity);
		auto& layout = *entity_data->layout;
		auto* initial_chunk = entity_data->address.chunk;

		REQUIRE(initial_chunk->GetNextChunk() == nullptr);

		for (int i = 0; i < layout.GetMaxEntityCount() - 1; i++)
			manager.CreateEntity();

		REQUIRE(initial_chunk->GetNextChunk() == nullptr); // filled all the available slots but still no next
		manager.CreateEntity();
		REQUIRE(initial_chunk->GetNextChunk() != nullptr); // Finally allocated the next chunk
		REQUIRE(initial_chunk->GetNextChunk()->GetEntityCount() == 1);

		manager.DestroyEntity(entity);
		manager.CreateEntity();
		REQUIRE(initial_chunk->GetNextChunk()->GetEntityCount() == 1); // Created in the recently freed place in 1st chunk
		manager.CreateEntity();
		REQUIRE(initial_chunk->GetNextChunk()->GetEntityCount() == 2); // Now in the 2nd

		REQUIRE(initial_chunk->GetNextChunk()->GetNextChunk() == nullptr);

		for (int i = 0; i < layout.GetMaxEntityCount(); i++)
			manager.CreateEntity();

		REQUIRE(initial_chunk->GetNextChunk()->GetNextChunk() != nullptr); // Now in the 2nd
	}

	SECTION("multiple component entity")
	{
		auto create_entity = [&]()
		{
			auto entity = manager.CreateEntity();
			manager.AddComponent<TestPositionComponent>(entity);
			manager.AddComponent<TestAABBComponent>(entity);
			return entity;
		};

		auto entity = create_entity();
		auto* entity_data = manager.GetComponent<EntityData>(entity);
		auto& layout = *entity_data->layout;
		auto* initial_chunk = entity_data->address.chunk;

		REQUIRE(initial_chunk->GetNextChunk() == nullptr);
		
		for (int i = 0; i < layout.GetMaxEntityCount() - 1; i++)
			create_entity();

		REQUIRE(initial_chunk->GetNextChunk() == nullptr); // filled all the available slots but still no next
		create_entity();
		REQUIRE(initial_chunk->GetNextChunk() != nullptr); // Finally allocated the next chunk
		REQUIRE(initial_chunk->GetNextChunk()->GetEntityCount() == 1);

		manager.DestroyEntity(entity);
		create_entity();
		REQUIRE(initial_chunk->GetNextChunk()->GetEntityCount() == 1); // Created in the recently freed place in 1st chunk
		create_entity();
		REQUIRE(initial_chunk->GetNextChunk()->GetEntityCount() == 2); // Now in the 2nd

		REQUIRE(initial_chunk->GetNextChunk()->GetNextChunk() == nullptr);

		for (int i = 0; i < layout.GetMaxEntityCount(); i++)
			create_entity();

		REQUIRE(initial_chunk->GetNextChunk()->GetNextChunk() != nullptr); // Now in the 2nd
	}

}