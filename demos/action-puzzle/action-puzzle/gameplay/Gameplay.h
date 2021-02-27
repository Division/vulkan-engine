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
		static constexpr double MAX_CONNECT_DISTANCE = 2.7f;
		static constexpr int MIN_POP_SELECTION = 3;
		static constexpr double POP_INTERVAL = 0.2;

		Gameplay();
		~Gameplay();
		void Update(float dt);
		void UpdatePhysics(float dt);
		void ClearEntities();
		void StartGame();

	private:
		typedef utils::SmallVector<ECS::EntityID, 10> RaycastResult;
		struct PopChain
		{
			std::vector<ECS::EntityID> ids;
			int position = 0;
			bool active = false;
		};

		void ApplyGravity();
		vec4 GetSphereColor(int index);
		void ProcessInput();
		void ProcessSelection(const RaycastResult& hits);
		void FinishSelection();
		void QueuePopSelection();
		void UpdatePopSelection();
		void PopEntity(ECS::EntityID id);

	private:
		class GameplayUtils;

		std::unique_ptr<GameplayUtils> utils;

		Resources::EntityResource::Handle basic_sphere;
		Resources::EntityResource::Handle attractor_sphere;
		std::unordered_set<ECS::EntityID> all_entities;
		Engine* engine;
		ECS::EntityManager* manager;
		ECS::TransformGraph* graph;
		Physics::PhysXManager* physics;


		vec3 gravity_center = vec3(0, 0, 0);
		int live_sphere_count = 0;
		int max_spheres = 0;
		bool selection_active = false;
		std::vector<ECS::EntityID> current_selection;
		int selection_color = 0;
		vec3 selection_position;

		// Queue of popping chains
		std::deque<PopChain> pop_chains;
		std::unordered_set<ECS::EntityID> non_selectable_entities;

		double last_spawn_time;
		double last_pop_time = 0;
	};

}