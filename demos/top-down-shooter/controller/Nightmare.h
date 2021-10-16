#pragma once

#include <scene/Behaviour.h>

namespace ECS::components
{
	struct AnimationController;
	struct Transform;
}

namespace scene
{
	class CharacterController;
	class MonsterController;

	class NightmareBehaviour : public Behaviour
	{
		static constexpr float CHASE_START_DISTANCE = 8.0f;
		static constexpr float CHASE_STOP_DISTANCE = 10.0f;

		enum class State
		{
			Idle,
			Chase,
			Attack
		};

		State state = State::Idle;

		double last_attack_time = 0.0;
		// Can store the ptr because we know the lifetime
		MonsterController* monster = nullptr;

		void ProcessIdle(float distance_to_player);
		void ProcessChase(float distance_to_player, vec3 player_position, vec3 position);

	public:

		void Awake() override;
		void Update(float dt) override;
	};

}