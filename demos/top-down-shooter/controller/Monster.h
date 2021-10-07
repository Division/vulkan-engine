#pragma once

#include <scene/Behaviour.h>
#include <magic_enum/magic_enum.hpp>
#include "resources/SkeletalAnimationResource.h"
#include "render/animation/SkeletalAnimation.h"

namespace ECS::components
{
	struct AnimationController;
	struct Transform;
}

namespace scene
{
	class MonsterController : public Behaviour
	{
	public:

		enum AnimationType
		{
			Idle,
			Move,
			Attack,
			Transition
		};

		enum class State
		{
			Idle,
			Move,
			Attack,
			Transition
		};

		typedef std::array<Resources::SkeletalAnimationResource::Handle, magic_enum::enum_count<AnimationType>()> Animations;

		void Initialize(Animations animations);

		void Awake() override;
		void Update(float dt) override;
		int ExecutionOrder() const override { return Behaviour::ExecutionOrder() + 5; }

		void SetMoveDirection(std::optional<vec2> value) { move_direction = value; };
		void SetTurnSpeed(float value) { turn_speed = value; };
		float GetTurnSpeed() const { return turn_speed; };
	
		void SetState(State value);

	private:
		void ProcessTurn(float dt);

	private:
		State state = State::Idle;
		std::optional<vec2> move_direction = std::nullopt;
		SkeletalAnimation::AnimationMixer* mixer = nullptr;
		bool initialized = false;
		vec2 current_aim_dir = vec2(0, 1);
		float turn_speed = 0.0f;
		Animations animations;
	};

}