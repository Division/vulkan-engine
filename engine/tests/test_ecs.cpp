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