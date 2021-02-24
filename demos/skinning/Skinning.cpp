#include "Skinning.h"
#include "Engine.h"
#include "camera/ViewerCamera.h"
#include "system/Input.h"
#include "ecs/components/Static.h"
#include "ecs/components/AnimationController.h"
#include "scene/Scene.h"
#include "render/debug/DebugDraw.h"
#include "render/texture/Texture.h"
#include "resources/MultiMesh.h"
#include "resources/EntityResource.h"
#include "resources/SkeletonResource.h"
#include "resources/SkeletalAnimationResource.h"
#include "system/Input.h"

Game::Game() = default;
Game::~Game() = default;

using namespace System;
using namespace ECS;
using namespace ECS::systems;
using namespace physx;

ECS::EntityID Game::CreateLight(vec3 position, float radius, ECS::components::Light::Type type, vec3 color)
{
	auto entity = manager->CreateEntity();

	auto* transform = manager->AddComponent<ECS::components::Transform>(entity);
	transform->position = position;

	auto* light = manager->AddComponent<ECS::components::Light>(entity);
	light->type = type;
	light->color = color;
	light->radius = radius;

	return entity;
}

void Game::init()
{
	OPTICK_EVENT();

	auto* engine = Engine::Get();
	manager = engine->GetEntityManager();
	graph = engine->GetTransformGraph();

	manager->AddStaticComponent(graph);

	CreateLight(vec3(5, 5, 5), 100, ECS::components::Light::Type::Point, vec3(1, 1, 1));

	camera = std::make_unique<ViewerCamera>();

	animated_entity = Resources::EntityResource::Handle(L"assets/Entities/Insect/insect.entity");
	animation = Resources::SkeletalAnimationResource::Handle(L"assets/art/Entities/Insect/Insect@Flying.ozz");
	//animation = Resources::SkeletalAnimationResource::Handle(L"assets/art/Entities/Insect/Take 001.ozz");

	animated_entity_id = animated_entity->Spawn(vec3(0, 0, 0));
	auto* controller = manager->GetComponent<components::AnimationController>(animated_entity_id);
	auto anim_instance = controller->mixer->PlayAnimation(animation, SkeletalAnimation::PlaybackMode::Loop);

	animated_entity_id = animated_entity->Spawn(vec3(5, 0, 0));
	controller = manager->GetComponent<components::AnimationController>(animated_entity_id);
	controller->mixer->PlayAnimation(animation, SkeletalAnimation::PlaybackMode::Loop);
}

void Game::UpdatePhysics(float dt)
{
}

void Game::update(float dt)
{
	Resources::Cache::Get().GCCollect();
	camera->Update(dt);

	Engine::Get()->GetDebugDraw()->DrawLine(vec3(), vec3(1, 0, 0), vec4(1, 0, 0, 1));
	Engine::Get()->GetDebugDraw()->DrawLine(vec3(), vec3(0, 1, 0), vec4(0, 1, 0, 1));
	Engine::Get()->GetDebugDraw()->DrawLine(vec3(), vec3(0, 0, 1), vec4(0, 0, 1, 1));
}

IGamePhysicsDelegate* Game::GetPhysicsDelegate()
{
	return this;
}

PxFilterFlags SimpleFilterShader
(PxFilterObjectAttributes attributes0, PxFilterData filterData0,
	PxFilterObjectAttributes attributes1, PxFilterData filterData1,
	PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
	pairFlags = PxPairFlag::eCONTACT_DEFAULT;

	return PxFilterFlags();
}

physx::PxSimulationFilterShader Game::GetFilterShader()
{
	return PxDefaultSimulationFilterShader;
}

physx::PxVec3 Game::GetGravity()
{
	return physx::PxVec3(0, 0, 0);
}

void Game::cleanup()
{
}