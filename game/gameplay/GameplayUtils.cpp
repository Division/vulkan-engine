#pragma once

#include "GameplayUtils.h"
#include "ecs/components/Physics.h"
#include "ecs/components/Transform.h"
#include "ecs/components/MultiMeshRenderer.h"

namespace game
{
	ECS::EntityID Gameplay::GameplayUtils::CreateSphere(vec3 position, quat rotation, components::Sphere::Type type, int color)
	{
		auto entity_id = gameplay.basic_sphere->Spawn(position);

		auto* sphere = gameplay.manager->AddComponent<components::Sphere>(entity_id);
		sphere->type = type;
		sphere->color = color;
		sphere->position = position;

		gameplay.live_sphere_count += 1;
		gameplay.all_entities.push_back(entity_id);

		auto* rigidbody = gameplay.manager->GetComponent<ECS::components::RigidbodyDynamic>(entity_id);
		rigidbody->body->setLinearDamping(0.5f);

		auto* renderer = gameplay.manager->GetComponent<ECS::components::MultiMeshRenderer>(entity_id);
		renderer->materials = render::MaterialList::Create();

		for (auto& material_resource : *renderer->material_resources)
		{
			auto material = material_resource->Get()->Clone();
			material->SetColor(gameplay.GetSphereColor(color));
			renderer->materials->push_back(material);
		}

		renderer->material_resources = nullptr;

		return entity_id;
	}

	ECS::EntityID Gameplay::GameplayUtils::CreateAttractor(vec3 position, quat rotation)
	{
		auto entity_id = gameplay.attractor_sphere->Spawn(position);

		auto* attractor = gameplay.manager->AddComponent<components::Attractor>(entity_id);
		attractor->position = position;

		gameplay.all_entities.push_back(entity_id);

		return entity_id;
	}

	ECS::EntityID Gameplay::GameplayUtils::CreateLight(vec3 position, float radius, ECS::components::Light::Type type, vec3 color)
	{
		auto entity = gameplay.manager->CreateEntity();

		auto* transform = gameplay.manager->AddComponent<ECS::components::Transform>(entity);
		transform->position = position;

		auto* light = gameplay.manager->AddComponent<ECS::components::Light>(entity);
		light->type = type;
		light->color = color;
		light->radius = radius;

		gameplay.all_entities.push_back(entity);

		return entity;
	}

	utils::SmallVector<ECS::EntityID, 10> Gameplay::GameplayUtils::Raycast(vec3 position, vec3 direction)
	{
		return {};
	}
}