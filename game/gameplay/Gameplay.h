#pragma once

#include "resources/EntityResource.h"
#include "ecs/ECS.h"
#include "scene/Physics.h"
#include "Engine.h"

namespace game
{

	class Gameplay
	{
	public:
		Gameplay();
		~Gameplay();
		void Update(float dt);
		void UpdatePhysics(float dt);
		void ClearEntities();
		void StartGame();

	private:
		void ApplyGravity();
		vec4 GetSphereColor(int index);

	private:
		class GameplayUtils;

		std::unique_ptr<GameplayUtils> utils;

		Resources::EntityResource::Handle basic_sphere;
		Resources::EntityResource::Handle attractor_sphere;
		std::vector<ECS::EntityID> all_entities;
		Engine* engine;
		ECS::EntityManager* manager;
		ECS::TransformGraph* graph;
		Physics::PhysXManager* physics;

		vec3 gravity_center = vec3(0, 0, 0);
		int live_sphere_count = 0;
		int max_spheres = 0;

		double last_spawn_time;
	};

}