#include "lib/catch/catch.hpp"
#include "ecs/ECS.h"
#include "ecs/TransformGraph.h"
#include "ecs/System.h"
#include <array>

using namespace core::ECS;
using namespace core::ECS::components;

TEST_CASE("ECS systems")
{
	Transform default_transform;

	EntityManager manager;
	TransformGraph graph(manager);

	auto parent = manager.CreateEntity();
	*manager.AddComponent<Transform>(parent) = default_transform;

	std::array<EntityID, 5> children;
	std::array<EntityID, 5> objects;
	for (auto& child : children)
	{
		child = manager.CreateEntity();
		graph.AddChild(parent, child);
		*manager.AddComponent<Transform>(child) = default_transform;
	}

	for (auto& child : objects)
	{
		child = manager.CreateEntity();
		*manager.AddComponent<Transform>(child) = default_transform;
	}

	auto root_transform_list = manager.GetChunkListsWithComponent<RootTransform>();
	auto non_child_transform_list = manager.GetChunkListsWithoutComponent<ChildTransform>();
	
	REQUIRE(root_transform_list.size() == 1);

	class EntityCountSystem : public System
	{
	public:
		EntityCountSystem(EntityManager& manager) : System(manager) {}

		void Process(Chunk* chunk)
		{
			counter += chunk->GetEntityCount();
		}

		int GetProcessedEntityCount() const { return counter; }

	private:
		int counter = 0;
	};

	EntityCountSystem count_root(manager);
	count_root.ProcessChunks(root_transform_list);
	REQUIRE(count_root.GetProcessedEntityCount() == 1);

	EntityCountSystem count_no_child(manager);
	count_no_child.ProcessChunks(non_child_transform_list);
	REQUIRE(count_no_child.GetProcessedEntityCount() == 6);
}