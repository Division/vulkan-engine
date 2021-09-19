#include "lib/catch/catch.hpp"
#include "ecs/ECS.h"
#include "ecs/TransformGraph.h"
#include "ecs/System.h"
#include "ecs/systems/TransformSystem.h"
#include <array>

using namespace ECS;
using namespace ECS::components;
using namespace ECS::systems;

TEST_CASE("ECS transform system")
{
	Transform default_transform;

	EntityManager manager;
	TransformGraph graph(manager);

	auto parent = manager.CreateEntity();
	auto* transform_parent = manager.AddComponent<Transform>(parent);
	*transform_parent = Transform();
	transform_parent->position = vec3(0, 5, 0);

	auto child = manager.CreateEntity();
	auto* transform_child = manager.AddComponent<Transform>(child);
	*transform_child = Transform();
	transform_child->position = vec3(1, 0, 0);

	auto child2 = manager.CreateEntity();
	auto* transform_child2 = manager.AddComponent<Transform>(child2);
	*transform_child2 = Transform();
	transform_child2->position = vec3(3, -1, 0);

	auto child_level2 = manager.CreateEntity();
	auto* transform_child_level2 = manager.AddComponent<Transform>(child_level2);
	*transform_child_level2 = Transform();
	transform_child_level2->position = vec3(5, 0, 0);

	graph.AddChild(parent, child);
	graph.AddChild(parent, child2);
	graph.AddChild(child, child_level2);

	NoChildTransformSystem no_child_system(graph, manager);
	RootTransformSystem root_transform_system(graph, manager);
	
	auto root_list = manager.GetChunkListsWithComponents<RootTransform, Transform>();
	auto no_child_list = manager.GetChunkLists([](ChunkList* chunk_list) {
		auto hash1 = GetComponentHash<ChildTransform>();
		auto hash2 = GetComponentHash<Transform>();
		return !chunk_list->HasComponent(hash1) && chunk_list->HasComponent(hash2);
	});

	no_child_system.ProcessChunks(no_child_list);
	root_transform_system.ProcessChunks(root_list);

	transform_child_level2 = manager.GetComponent<Transform>(child_level2);
	auto transformed_vertex = transform_child_level2->GetLocalToWorld() * vec4(0, 0, 0, 1);
	REQUIRE(transformed_vertex == vec4(6, 5, 0, 1));
	
	transform_child2 = manager.GetComponent<Transform>(child2);
	transformed_vertex = transform_child2->GetLocalToWorld() * vec4(0, 0, 0, 1);
	REQUIRE(transformed_vertex == vec4(3, 4, 0, 1));
}