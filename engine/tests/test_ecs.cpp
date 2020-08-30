#include <functional>
#include "lib/catch/catch.hpp"
#include "ecs/ECS.h"
#include "ecs/CommandBuffer.h"

using namespace ECS;

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

	auto entity = manager.CreateEntity();
	manager.AddComponent<TestAABBComponent>(entity);

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
		auto* initial_chunk = manager.GetEntityAddressMap().at(entity).chunk;
		auto& layout = initial_chunk->GetComponentLayout();

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
		auto* initial_chunk = manager.GetEntityAddressMap().at(entity).chunk;
		auto& layout = initial_chunk->GetComponentLayout();

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

TEST_CASE("ECS component constructor/destructor/move") 
{
	bool destroyed = false;

	struct TestComponent
	{
		TestComponent() = default;
		~TestComponent()
		{
			if (destroy_callback)
				destroy_callback();
		}

		TestComponent(TestComponent&& other)
		{
			destroy_callback = std::move(other.destroy_callback);
			move_callback = std::move(other.move_callback);
			startup_value = std::move(other.startup_value);
			if (move_callback)
				move_callback();
		}

		void SetDestroyCallback(std::function<void()> destroy_callback) { this->destroy_callback = destroy_callback; };
		void SetMoveCallback(std::function<void()> move_callback) { this->move_callback = move_callback; };

		int startup_value = 123;
		std::function<void()> destroy_callback;
		std::function<void()> move_callback;
	};

	EntityManager manager;
	auto& address_map = manager.GetEntityAddressMap();
	auto& chunk_map = manager.GetChunkMap();

	auto entity = manager.CreateEntity();
	auto* component = manager.AddComponent<TestComponent>(entity);
	component->SetDestroyCallback([&]{ destroyed = true; });

	// Destroying entity
	manager.DestroyEntity(entity);
	REQUIRE(component->startup_value == 123);
	REQUIRE(destroyed == true);

	destroyed = false;
	entity = manager.CreateEntity();
	component = manager.AddComponent<TestComponent>(entity);
	component->SetDestroyCallback([&]{ destroyed = true; });
	manager.RemoveComponent<TestComponent>(entity);
	REQUIRE(destroyed == true);

	destroyed = false;
	{
		EntityManager temp_manager;
		entity = temp_manager.CreateEntity();
		component = temp_manager.AddComponent<TestComponent>(entity);
		component->SetDestroyCallback([&]{ destroyed = true; });
	}
	REQUIRE(destroyed == true);

	// Move constructor
	bool moved = false;
	auto move_callback = [&] { moved = true; };

	{
		EntityManager temp_manager;
		entity = temp_manager.CreateEntity();
		component = temp_manager.AddComponent<TestComponent>(entity);
		component->SetMoveCallback(move_callback);
		temp_manager.AddComponent<TestPositionComponent>(entity);
		REQUIRE(moved == true);

		auto entity2 = temp_manager.CreateEntity();
		temp_manager.AddComponent<TestPositionComponent>(entity2);
		
		component = temp_manager.AddComponent<TestComponent>(entity2);
		component->SetMoveCallback(move_callback);
		
		moved = false;
		temp_manager.DestroyEntity(entity);
		REQUIRE(moved == true);
	}

	struct ParamsComponent
	{
		ParamsComponent(int a, const std::string& b)
			: a(a), b(b)
		{}

		int a;
		std::string b;
	};

	{
		EntityManager temp_manager;
		entity = temp_manager.CreateEntity();
		auto component = temp_manager.AddComponent<ParamsComponent>(entity, 321, "test");
		REQUIRE(component->a == 321);
		REQUIRE(component->b == "test");
	}
}

TEST_CASE("ECS command buffer")
{
	int counter = 0;

	struct TestComponent
	{
		TestComponent(std::string str, int* counter) : counter(counter), str(str)
		{
			if (counter) *counter += 1;
		}

		TestComponent(const TestComponent&) = delete;

		~TestComponent()
		{
			if (counter) *counter -= 1;
		}

		TestComponent(TestComponent&& other) noexcept
		{
			this->counter = other.counter;
			other.counter = nullptr;
			this->str = std::move(other.str);
		}

		std::string str;
		int* counter;
	};

	EntityManager manager;

	EntityID entity1 = manager.CreateEntity();
	EntityID entity2 = manager.CreateEntity();

	CommandBuffer command_buffer(manager);
	command_buffer.AddComponent(entity1, TestComponent("some_text1", &counter));
	command_buffer.AddComponent(entity2, TestComponent("some_text2", &counter));
	command_buffer.Flush();

	REQUIRE(counter == 2);
	
	auto* component1 = manager.GetComponent<TestComponent>(entity1);
	auto* component2 = manager.GetComponent<TestComponent>(entity2);
	REQUIRE(component1->str == "some_text1");
	REQUIRE(component2->str == "some_text2");
	
	command_buffer.RemoveComponent<TestComponent>(entity1);
	REQUIRE(counter == 2);
	command_buffer.Flush();
	REQUIRE(counter == 1);
	command_buffer.DestroyEntity(entity2);
	command_buffer.DestroyEntity(entity1);
	REQUIRE(manager.EntityExists(entity1));
	command_buffer.Flush();
	REQUIRE(counter == 0);
	REQUIRE(!manager.EntityExists(entity2));

	// Transient entity

	auto transient1 = command_buffer.CreateEntity();
	auto transient2 = command_buffer.CreateEntity();
	command_buffer.AddComponent(transient1, TestComponent("text3", &counter));
	command_buffer.AddComponent(transient2, TestComponent("text4", &counter));
	command_buffer.RemoveComponent<TestComponent>(transient2);
	REQUIRE(counter == 2);
	command_buffer.Flush();
	REQUIRE(counter == 1);
}