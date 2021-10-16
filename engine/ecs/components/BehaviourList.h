#pragma once

#include <utils/DataStructures.h>
#include <ecs/ECS.h>
#include "Entity.h"
#include <vector>
#include "scene/Behaviour.h";
#include "AnimationController.h"

namespace ECS::components 
{

	struct BehaviourList
	{
		std::vector<std::shared_ptr<scene::Behaviour>> behaviours;
		EntityManager* manager = nullptr;
		EntityID id = 0;
		SkeletalAnimation::Dispatcher::Handle animation_callback_handle;

		void SortBehaviours()
		{
			std::sort(behaviours.begin(), behaviours.end(), [](const std::shared_ptr<scene::Behaviour>& a, const std::shared_ptr<scene::Behaviour>& b) { return a->ExecutionOrder() < b->ExecutionOrder(); });
		}

		void AddBehaviour(std::unique_ptr<scene::Behaviour> behaviour)
		{
			// TODO: check for existance
			behaviours.push_back(std::move(behaviour));
			SortBehaviours();
		}

		std::shared_ptr<scene::Behaviour> GetBehaviour(size_t hash) const
		{
			auto it = std::find_if(behaviours.begin(), behaviours.end(), [hash](const std::shared_ptr<scene::Behaviour>& b) { return hash == b->GetHash(); });
			return it == behaviours.end() ? nullptr : *it;
		}

		void InitializeBehaviour(scene::Behaviour& behaviour)
		{
			behaviour.behaviour_list = this;
			behaviour.manager = manager;
			behaviour.id = id;
			behaviour.hash = typeid(behaviour).hash_code(); // for polymorphic classes it should be different from the base class
			assert(behaviour.hash != typeid(scene::Behaviour).hash_code()); // Don't want the base Behaviour itself to be added
		}


		static void Initialize(EntityManager& manager, EntityID id, BehaviourList* behaviour_list)
		{
			behaviour_list->id = id;
			behaviour_list->manager = &manager;

			auto controller = manager.GetComponent<AnimationController>(id);
			if (controller)
			{
				behaviour_list->animation_callback_handle = controller->mixer->AddCallback([id, &manager](SkeletalAnimation::EventType type, SkeletalAnimation::EventParam param) {
					auto behaviour_list = manager.GetComponent<BehaviourList>(id);
					if (behaviour_list)
						behaviour_list->TriggerAnimationEvent(type, param);
				});
			}

			for (auto& behaviour : behaviour_list->behaviours)
				behaviour_list->InitializeBehaviour(*behaviour);
			
			for (auto& behaviour : behaviour_list->behaviours)
				behaviour->Awake();
		}

		void Update(float dt)
		{
			for (auto& behaviour : behaviours)
				behaviour->Update(dt);
		}

		void LateUpdate(float dt)
		{
			for (auto& behaviour : behaviours)
				behaviour->LateUpdate(dt);
		}

		void UpdatePhysics(float dt)
		{
			for (auto& behaviour : behaviours)
				behaviour->UpdatePhysics(dt);
		}

		void TriggerAnimationEvent(SkeletalAnimation::EventType type, SkeletalAnimation::EventParam param)
		{
			for (auto& behaviour : behaviours)
				behaviour->OnAnimationEvent(type, param);
		}

		void Deinitialize()
		{
			for (auto& behaviour : behaviours)
				behaviour->Deinitialize();
		}
	};

}