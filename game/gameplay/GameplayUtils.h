#pragma once

#include "Gameplay.h"
#include "components/Sphere.h"
#include "components/Attractor.h"
#include "ecs/ECS.h"
#include "ecs/components/Light.h"

namespace game
{
	class Gameplay::GameplayUtils
	{
	private:
		Gameplay& gameplay;

	public:
		GameplayUtils(Gameplay& gameplay) : gameplay(gameplay) {}

		ECS::EntityID CreateSphere(vec3 position, quat rotation, components::Sphere::Type type, int color);
		ECS::EntityID CreateAttractor(vec3 position, quat rotation);
		ECS::EntityID CreateLight(vec3 position, float radius, ECS::components::Light::Type type, vec3 color);
	};
}