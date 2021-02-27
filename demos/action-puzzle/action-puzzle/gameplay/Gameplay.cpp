#include "Gameplay.h"
#include "scene/Scene.h"
#include "GameplayUtils.h"
#include "ecs/components/Physics.h"
#include "ecs/System.h"
#include "utils/Math.h"
#include "objects/Camera.h"
#include "system/Input.h"
#include "render/debug/DebugDraw.h"

using namespace ECS;
using namespace physx;

namespace game
{
	constexpr double SPAWN_INTERVAL = 0.1;

	Gameplay::Gameplay()
	{
		basic_sphere = Resources::EntityResource::Handle(L"assets/Entities/Sphere/sphere.entity");
		attractor_sphere = Resources::EntityResource::Handle(L"assets/Entities/Sphere/sphere_attractor.entity");
		engine = Engine::Get();
		manager = engine->GetEntityManager();
		graph = engine->GetTransformGraph();
		physics = engine->GetScene()->GetPhysics();

		utils = std::make_unique<GameplayUtils>(*this);
	}

	Gameplay::~Gameplay() = default;
	
	vec4 Gameplay::GetSphereColor(int index)
	{
		const vec4 colors[] = { vec4(1, 0, 0, 1), vec4(0, 1, 0, 1) , vec4(1, 0, 1, 1) , vec4(0, 1, 1, 1) };
		if (index >= std::size(colors))
			return colors[0];

		return colors[index];
	}

	void Gameplay::ClearEntities()
	{
		for (auto id : all_entities)
			manager->DestroyEntity(id);
		all_entities.clear();
		live_sphere_count = 0;
	}

	void Gameplay::StartGame()
	{
		ClearEntities();
		max_spheres = 100;
		last_spawn_time = 0;

		auto* camera = Engine::Get()->GetScene()->GetCamera();
		camera->Transform().position = vec3(0, 10, -25);

		utils->CreateAttractor(vec3(0, 0, 0), quat());
		utils->CreateLight(vec3(1, 15, -7), 30, ECS::components::Light::Type::Point, vec3(1, 1, 1));
	}

	void Gameplay::UpdatePhysics(float dt)
	{
		ApplyGravity();
	}

	void Gameplay::ApplyGravity()
	{
		auto chunks = manager->GetChunkListsWithComponents<components::Sphere, ECS::components::RigidbodyDynamic>();

		const PxVec3 px_gravity_center = Physics::Convert(gravity_center);

		CallbackSystem([px_gravity_center](Chunk* chunk) {
			ComponentFetcher<ECS::components::RigidbodyDynamic> rigidbody_dynamic_fetcher(*chunk);
			ComponentFetcher<components::Sphere> sphere_fetcher(*chunk);

			for (int i = 0; i < chunk->GetEntityCount(); i++)
			{
				auto* rigidbody = rigidbody_dynamic_fetcher.GetComponent(i);
				auto* sphere = sphere_fetcher.GetComponent(i);
				auto transform = rigidbody->body->getGlobalPose();
				auto direction = (px_gravity_center - transform.p).getNormalized();
				rigidbody->body->addForce(direction * 15.0f, PxForceMode::eACCELERATION, false);
				sphere->position = Physics::Convert(transform.p); // Update position as well
			}

		}, *manager, false).ProcessChunks(chunks);
	}

	void Gameplay::Update(float dt)
	{
		const bool can_spawn = engine->Get()->time() - last_spawn_time > SPAWN_INTERVAL && live_sphere_count < max_spheres;
		if (can_spawn)
		{
			last_spawn_time = engine->Get()->time();
			auto entity_id = utils->CreateSphere(vec3(25, 15, 3 * Random()), quat(), components::Sphere::Type::Default, (int)(Random() * 4));

			auto* rigidbody = manager->GetComponent<ECS::components::RigidbodyDynamic>(entity_id);
			rigidbody->body->setLinearVelocity(PxVec3(-30, 0, 0));
		}

		ProcessInput();
		UpdatePopSelection();
	}

	void Gameplay::ProcessInput()
	{
		auto input = engine->GetInput();
		if (input->keyDown(::System::Key::MouseLeft))
		{
			auto* camera = Engine::Get()->GetScene()->GetCamera();
			auto ray = camera->GetMouseRay(input->mousePosition());

			auto hits = utils->Raycast(ray.first, ray.second);
			if (hits.size())
			{
				ProcessSelection(hits);
			}

			selection_active = true;
		}
		else if (selection_active)
		{
			FinishSelection();
			selection_active = false;
		}

		//auto debug_draw = engine->GetDebugDraw();
		//debug_draw->DrawLine(debug_start, debug_start + debug_dir * 100.0f, vec4(1, 0, 0, 1));
	}

	void Gameplay::ProcessSelection(const RaycastResult& hits)
	{

		for (auto id : hits)
		{
			auto iter = std::find(current_selection.begin(), current_selection.end(), id);
			if (iter == current_selection.end())
			{
				utils->Select(id);
			}
			else if (current_selection.size() > 1)
			{
				if (*iter == current_selection[current_selection.size() - 2])
				{
					utils->Deselect(current_selection.back());
				}
			}
		}
	}

	void Gameplay::FinishSelection()
	{
		if (current_selection.size() < MIN_POP_SELECTION)
		{
			for (int i = current_selection.size() - 1; i >= 0; i--)
			{
				utils->Deselect(current_selection[i]);
			}
		}
		else
		{
			QueuePopSelection();
		}
	}

	void Gameplay::QueuePopSelection()
	{
		if (current_selection.size() >= MIN_POP_SELECTION)
		{
			PopChain chain;
			for (auto id : current_selection)
			{
				non_selectable_entities.insert(id);
				chain.ids.push_back(id);
			}

			pop_chains.emplace_back(std::move(chain));
			current_selection.clear();
		}
	}

	void Gameplay::UpdatePopSelection()
	{
		if (pop_chains.empty())
			return;

		bool can_pop = engine->Get()->time() - last_pop_time > POP_INTERVAL;
		while (can_pop && pop_chains.size() > 0)
		{
			auto& first = pop_chains.front();
			if (first.position >= first.ids.size())
			{
				pop_chains.pop_front();
			}
			else
			{
				PopEntity(first.ids[first.position]);
				first.position += 1;
				can_pop = false;
				last_pop_time = engine->Get()->time();
			}
		}
	}

	void Gameplay::PopEntity(ECS::EntityID id)
	{
		manager->DestroyEntity(id);
		non_selectable_entities.erase(id);
		all_entities.erase(id);
		live_sphere_count -= 1;
	}


}