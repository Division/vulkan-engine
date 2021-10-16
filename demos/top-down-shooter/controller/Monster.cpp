#include "Monster.h"
#include "resources/SkeletalAnimationResource.h"
#include <ecs/components/AnimationController.h>
#include <ecs/components/Transform.h>
#include "TopDownShooter.h"
#include "Engine.h"

using namespace Resources;
using namespace ECS;

namespace scene
{
	void MonsterController::Initialize(Animations _animations)
	{
		animations = _animations;

		auto animation_controller = GetComponent<components::AnimationController>();
		animation_controller->mixer->PlayAnimation(animations[MonsterController::AnimationType::Idle], SkeletalAnimation::PlaybackParams().Loop());

		initialized = true;
	}

	void MonsterController::Awake()
	{
		auto animation_controller = GetComponent<components::AnimationController>();
		mixer = animation_controller->mixer.get();
	}

	void MonsterController::ProcessTurn(float dt)
	{
		if (!move_direction)
			return;
	}

	void MonsterController::SetState(State value)
	{
		if (state == value)
			return;

		state = value;

		switch (state)
		{
		case State::Idle:
		{
			mixer->PlayAnimation(animations[AnimationType::Idle], SkeletalAnimation::PlaybackParams().Loop());
			break;
		}
		case State::Move:
		{
			mixer->PlayAnimation(animations[AnimationType::Move], SkeletalAnimation::PlaybackParams().Loop());
			break;
		}
		case State::Attack:
		{
			mixer->PlayAnimation(animations[AnimationType::Attack], SkeletalAnimation::PlaybackParams().Once());
			break;
		}
		}
	}

	void MonsterController::Update(float dt)
	{
		if (!initialized)
			return;

		if (turn_speed > 0.001f)
			ProcessTurn(dt);


	}

}