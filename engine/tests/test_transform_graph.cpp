#include "lib/catch/catch.hpp"
#include "ecs/ECS.h"
#include "ecs/TransformGraph.h"
#include <array>

using namespace ECS;
using namespace ECS::components;

TEST_CASE("ECS transform graph")
{
	EntityManager manager;
	TransformGraph graph(manager);

	auto parent = manager.CreateEntity();
	std::array<EntityID, 5> children;
	std::array<EntityID, 5> subchildren;
	for (auto& child : children)
	{
		child = manager.CreateEntity();
		graph.AddChild(parent, child);
	}

	for (auto& child : subchildren)
	{
		child = manager.CreateEntity();
		graph.AddChild(children[1], child);
	}

	REQUIRE(manager.GetComponent<RootTransform>(parent) != nullptr);
	REQUIRE(manager.GetComponent<RootTransform>(children[0]) == nullptr);
	REQUIRE(manager.GetComponent<ChildTransform>(children[0]) != nullptr);
	REQUIRE(manager.GetComponent<ChildTransform>(subchildren[0]) != nullptr);
	REQUIRE(manager.GetComponent<RootTransform>(children[1]) == nullptr);

	manager.DestroyEntity(parent);
	REQUIRE(manager.GetComponent<RootTransform>(children[1]) != nullptr);
	REQUIRE(manager.GetComponent<RootTransform>(subchildren[0]) == nullptr);
	REQUIRE(manager.GetComponent<ChildTransform>(subchildren[0]) != nullptr);

	manager.DestroyEntity(children[1]);
	REQUIRE(manager.GetComponent<RootTransform>(subchildren[0]) == nullptr);
	
	graph.AddChild(subchildren[0], subchildren[1]);
	REQUIRE(manager.GetComponent<RootTransform>(subchildren[0]) != nullptr);
}