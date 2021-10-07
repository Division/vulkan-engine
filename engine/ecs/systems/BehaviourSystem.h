#pragma once

#include <ecs/System.h>
#include <ecs/components/BehaviourList.h>
#include <ecs/components/Static.h>

namespace ECS::systems
{
	class BehaviourSystem : public System
	{
		std::vector<components::BehaviourList*> list;

	public:
		BehaviourSystem(EntityManager& manager) : System(manager) {}
		
		void BehaviourSystem::Process(Chunk* chunk)
		{
			OPTICK_EVENT();

			ComponentFetcher<components::BehaviourList> behaviour_fetcher(*chunk);

			for (int i = 0; i < chunk->GetEntityCount(); i++)
			{
				auto* behaviour = behaviour_fetcher.GetComponent(i);
				list.push_back(behaviour);
			}
		}

		void Update()
		{
			list.clear();

			ProcessChunks(manager.GetChunkListsWithComponent<components::BehaviourList>());
			auto dt = manager.GetStaticComponent<components::DeltaTime>()->dt;

			for (auto behaviour : list)
				behaviour->Update(dt);
		}

	};
}