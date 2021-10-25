#pragma once

#include "ecs/ECS.h"
#include "render/animation/SkeletalAnimation.h"

namespace ECS::components
{
	struct BehaviourList;
}

namespace scene
{
	class Behaviour
	{
	public:
		static constexpr int DEFAULT_ORDER = 1000;
		friend struct ECS::components::BehaviourList;

		virtual ~Behaviour() = default;

		template<typename T>
		T* GetComponent()
		{
			return manager->GetComponent<T>(id);
		}

		template<typename T, typename ...Args>
		T* AddComponent(Args&& ...args)
		{
			return manager->AddComponent<T>(id, std::forward<Args>(args)...);
		}

		template<typename T>
		std::weak_ptr<T> GetBehaviour()
		{
			static_assert(std::is_base_of<Behaviour, T>::value, "Parameter must be a Behaviour subclass");
			static_assert(!std::is_same<Behaviour, T>::value, "Parameter can't be a base Behaviour");
			return std::static_pointer_cast<T>(GetBehaviour(typeid(T).hash_code()));
		}

		template<typename T>
		std::shared_ptr<T> GetBehaviourStrong()
		{
			static_assert(std::is_base_of<Behaviour, T>::value, "Parameter must be a Behaviour subclass");
			static_assert(!std::is_same<Behaviour, T>::value, "Parameter can't be a base Behaviour");
			auto result = GetBehaviour(typeid(T).hash_code());
			if (!result)
				throw std::runtime_error("Behaviour doesn't exist");

			return std::static_pointer_cast<T>(result);
		}

		virtual int ExecutionOrder() const { return DEFAULT_ORDER; }
		virtual void Update(float dt) {}
		virtual void LateUpdate(float dt) {}
		virtual void UpdatePhysics(float dt) {}
		virtual void Awake() {}
		virtual void Deinitialize() {}
		virtual void OnAnimationEvent(SkeletalAnimation::EventType type, SkeletalAnimation::EventParam param) {};

		ECS::EntityID GetEntity() { return id; }

		size_t GetHash() const { return hash; }

	protected:
		ECS::EntityManager& GetManager() { return *manager; }

	private:
		std::shared_ptr<Behaviour> GetBehaviour(size_t hash);

		ECS::components::BehaviourList* behaviour_list = nullptr;
		ECS::EntityManager* manager = nullptr;
		ECS::EntityID id = 0;
		size_t hash = 0;
	};
}