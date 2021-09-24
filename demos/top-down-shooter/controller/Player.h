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
		void Shoot(ECS::components::Transform* player_transform, CharacterController* character_controller);

	public:

		void Awake() override;
		void Update(float dt) override;
	};

}