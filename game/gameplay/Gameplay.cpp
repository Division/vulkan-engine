#include "Gameplay.h"
#include "scene/Scene.h"
#include "GameplayUtils.h"
#include "ecs/components/Physics.h"
#include "ecs/System.h"
#include "utils/Math.h"
#include "objects/Camera.h"

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

	}

}