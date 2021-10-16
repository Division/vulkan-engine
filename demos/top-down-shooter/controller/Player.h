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

	class PlayerBehaviour : public Behaviour
	{
		double last_shoot_time = 0.0;

	private:
		void Shoot(vec3 position, CharacterController* character_controller);
		bool should_shoot = false;

	public:

		void Awake() override;
		void Update(float dt) override;
		void LateUpdate(float dt) override;
		void OnAnimationEvent(SkeletalAnimation::EventType type, SkeletalAnimation::EventParam param) override;
	};

}