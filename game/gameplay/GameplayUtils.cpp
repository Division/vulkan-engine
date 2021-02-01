#pragma once

#include "GameplayUtils.h"
#include "ecs/components/Physics.h"
#include "ecs/components/Transform.h"
#include "ecs/components/MultiMeshRenderer.h"

using namespace physx;

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
		gameplay.all_entities.insert(entity_id);

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

		gameplay.all_entities.insert(entity_id);

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

		gameplay.all_entities.insert(entity);

		return entity;
	}

	Gameplay::RaycastResult Gameplay::GameplayUtils::Raycast(vec3 position, vec3 direction)
	{
		PxScene* scene = gameplay.physics->GetScene();
		PxVec3 origin = Physics::Convert(position);
		PxVec3 unitDir = Physics::Convert(direction);
		PxReal maxDistance = 1000;
		PxRaycastBuffer hit;

		Gameplay::RaycastResult result;

		// Raycast against all static & dynamic objects (no filtering)
		// The main result from this call is the closest hit, stored in the 'hit.block' structure
		bool status = scene->raycast(origin, unitDir, maxDistance, hit);
		if (status)
		{
			auto id = (ECS::EntityID)hit.block.actor->userData;
			if (gameplay.manager->GetComponent<components::Sphere>(id))
				result.push_back(id);
		}

		return result;
	}

	bool Gameplay::GameplayUtils::Select(ECS::EntityID id)
	{
		const bool selection_empty = gameplay.current_selection.empty();
		auto* sphere = gameplay.manager->GetComponent<components::Sphere>(id);

		if (selection_empty)
		{
			gameplay.selection_color = sphere->color;
			gameplay.selection_position = sphere->position;
		}

		const bool color_match = sphere->color == gameplay.selection_color;
		const bool distance_match = glm::distance2(gameplay.selection_position, sphere->position) <= Gameplay::MAX_CONNECT_DISTANCE * Gameplay::MAX_CONNECT_DISTANCE;
		const bool ignore_match = gameplay.non_selectable_entities.find(id) == gameplay.non_selectable_entities.end();

		if (!color_match || !distance_match || !ignore_match)
			return false;

		auto* renderer = gameplay.manager->GetComponent<ECS::components::MultiMeshRenderer>(id);
		for (auto& material : *renderer->materials)
		{
			material->SetColor(vec4(1, 1, 1, 1));
			material->LightingEnabled(false);
			renderer->draw_calls.Reset(); // Need to recreate draw calls because shader changed
		}

		gameplay.selection_position = sphere->position;
		gameplay.current_selection.push_back(id);

		return true;
	}

	void Gameplay::GameplayUtils::Deselect(ECS::EntityID id)
	{
		auto iter = std::find(gameplay.current_selection.begin(), gameplay.current_selection.end(), id);

		assert(iter != gameplay.current_selection.end());
		if (iter == gameplay.current_selection.end())
			return;

		gameplay.current_selection.erase(iter);

		auto* sphere = gameplay.manager->GetComponent<components::Sphere>(id);
		auto* renderer = gameplay.manager->GetComponent<ECS::components::MultiMeshRenderer>(id);
		for (auto& material : *renderer->materials)
		{
			material->SetColor(gameplay.GetSphereColor(sphere->color));
			material->LightingEnabled(true);
			renderer->draw_calls.Reset(); // Need to recreate draw calls because shader changed
		}
	}
}