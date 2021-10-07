#pragma once

#include <scene/Behaviour.h>
#include "TopDownShooter.h"
#include "ecs/components/Transform.h"

namespace scene
{

	class Ground : public Behaviour
	{
		ECS::components::Transform* transform;

	public:
		void Awake() override
		{
			transform = GetComponent<ECS::components::Transform>();
		}

		void Update(float dt) override
		{
			transform->position = Game::GetInstance()->GetPlayerPosition();
			transform->position.y = 0;
		}
	};

}