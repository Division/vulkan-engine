#include "TopDownShooter.h"
#include "Engine.h"
#include "camera/ViewerCamera.h"
#include "system/Input.h"
#include "ecs/components/Static.h"
#include "ecs/components/AnimationController.h"
#include "ecs/components/BoneAttachment.h"
#include "ecs/components/BehaviourList.h"
#include "scene/Scene.h"
#include "render/debug/DebugDraw.h"
#include "render/texture/Texture.h"
#include "render/renderer/SceneRenderer.h"
#include "resources/MultiMesh.h"
#include "resources/EntityResource.h"
#include "resources/SkeletonResource.h"
#include "resources/SkeletalAnimationResource.h"
#include "system/Input.h"
#include "objects/Camera.h"
#include "utils/Math.h"
#include "render/renderer/EnvironmentSettings.h"
#include "controller/Controller.h"
#include "controller/Player.h"
#include "controller/Monster.h"
#include "controller/Nightmare.h"
#include "projectile/Projectile.h"
#include "objects/Ground.h"

Game::Game() = default;
Game::~Game() = default;

using namespace System;
using namespace ECS;
using namespace ECS::systems;
using namespace physx;
using namespace Resources;

IGame::InitParams Game::GetInitParams() const
{
	return { 1600, 1000, "Top Down Shooter demo" };
}

ECS::EntityID Game::CreateLight(vec3 position, float radius, ECS::components::Light::Type type, vec3 color)
{
	auto entity = manager->CreateEntity();

	auto* transform = manager->AddComponent<ECS::components::Transform>(entity);
	transform->position = position;

	auto* light = manager->AddComponent<ECS::components::Light>(entity);
	light->type = type;
	light->color = color;
	light->radius = radius;
	light->cast_shadows = true;

	return entity;
}

ECS::EntityID Game::CreatePlayer()
{
	auto player_template = Resources::EntityResource::Handle(L"assets/top-down-shooter/characters/uetest/player.entity");
	auto player_id = player_template->Spawn();

	auto behaviour = manager->AddComponent<components::BehaviourList>(player_id);
	behaviour->AddBehaviour(std::make_unique<scene::PlayerBehaviour>());
	behaviour->AddBehaviour(std::make_unique<scene::CharacterController>());

	auto rifle_handle = Resources::EntityResource::Handle(L"assets/top-down-shooter/characters/uetest/m4_rifle.entity");
	rifle_id = rifle_handle->Spawn(vec3(0, 0, 0));
	auto rifle_transform = manager->GetComponent<components::Transform>(rifle_id);
	rifle_transform->position = vec3(0, -0.01, 0);

	auto animation_controller = manager->GetComponent<components::AnimationController>(player_id);
	auto bone_attachment = manager->AddComponent<components::BoneAttachment>(rifle_id);
	bone_attachment->entity_id = player_id;
	bone_attachment->joint_index = animation_controller->mixer->GetSkeleton()->GetJointIndex("ik_hand_gun");

	return player_id;
}

ECS::EntityID Game::CreateNightmare(vec3 position)
{
	auto monster_template = Resources::EntityResource::Handle(L"assets/top-down-shooter/characters/monsters/nightmare/nightmare.entity");
	auto monster_id = monster_template->Spawn(position);

	auto behaviour = manager->AddComponent<components::BehaviourList>(monster_id);
	behaviour->AddBehaviour(std::make_unique<scene::MonsterController>());
	behaviour->AddBehaviour(std::make_unique<scene::NightmareBehaviour>());

	return monster_id;
}

void Game::init()
{
	OPTICK_EVENT();

	auto* settings = Engine::Get()->GetSceneRenderer()->GetEnvironmentSettings();
	settings->directional_light->enabled = true;
	settings->directional_light->zNear = 10;

	instance = this;

	auto* engine = Engine::Get();
	manager = engine->GetEntityManager();
	graph = engine->GetTransformGraph();

	engine->GetSceneRenderer()->SetIrradianceCubemap(Resources::TextureResource::Handle(L"assets/Textures/environment/IBL/irradiance3.ktx"));
	engine->GetSceneRenderer()->SetRadianceCubemap(Resources::TextureResource::Handle(L"assets/Textures/environment/IBL/radiance3.ktx"));

	player_id = CreatePlayer();

	manager->AddStaticComponent(graph);

	projectile_manager = std::make_unique<Projectile::ProjectileManager>(*manager);

	//point_light_id = CreateLight(vec3(2.5, 4, 0), 10, ECS::components::Light::Type::Point, vec3(1, 1, 1) * 10.0f);

	camera = std::make_unique<ViewerCamera>();

	auto plane_handle = Resources::EntityResource::Handle(L"assets/top-down-shooter/ground/ground.entity");
	auto ground_id = plane_handle->Spawn(vec3(0));
	auto ground_behaviour = manager->AddComponent<components::BehaviourList>(ground_id);
	ground_behaviour->AddBehaviour(std::make_unique<scene::Ground>());

	auto box_handle = Resources::EntityResource::Handle(L"assets/top-down-shooter/characters/uetest/phys_box.entity");
	box_handle->Spawn(vec3(5, 0, 5));

	engine->GetScene()->GetPhysics()->GetControllerManager()->setOverlapRecoveryModule(true);

	CreateNightmare(vec3(3, 0, -3));
}

void Game::UpdatePhysics(float dt)
{
}

std::optional<vec3> Game::GetMouseTarget() const
{
	auto input = Engine::Get()->GetInput();
	auto camera = Engine::Get()->GetScene()->GetCamera();
	auto ray = camera->GetMouseRay(input->mousePosition());

	Plane plane{ vec3(0,0,0), vec3(0,1,0) };
	vec3 intersection;
	if (plane.IntersectRay(ray.first, ray.second, &intersection))
		return intersection;

	return std::nullopt;
}

void Game::UpdateFollowCamera()
{
	auto camera = Engine::Get()->GetScene()->GetCamera();

	auto player_transform = manager->GetComponent<components::Transform>(player_id);

	camera->Transform().position = player_transform->position + vec3(0, 10, 5);
	camera->Transform().LookAt(player_transform->position);
}

void Game::update(float dt)
{
	Resources::Cache::Get().GCCollect();

	projectile_manager->Update();

	auto input = Engine::Get()->GetInput();
	if (input->FirstKeyDown(Key::Tab))
		camera_control = !camera_control;

	if (camera_control)
		camera->Update(dt);
	else
		UpdateFollowCamera();

	auto* settings = Engine::Get()->GetSceneRenderer()->GetEnvironmentSettings();
	auto player_transform = manager->GetComponent<components::Transform>(player_id);
	settings->directional_light->transform.position = player_transform->position + vec3(-10, 30, -20);
	settings->directional_light->orthographic_size = vec2(20, 20);
	settings->directional_light->transform.LookAt(
		player_transform->position,
		vec3(0, 1, 0)
	);

	last_player_position = player_transform->position;

	Engine::Get()->GetDebugDraw()->DrawAxis(vec3());

	/*if (input->keyDown(Key::Space))
	{
		auto light_transform = manager->GetComponent<components::Transform>(point_light_id);
		light_transform->position = Engine::Get()->GetScene()->GetCamera()->cameraPosition();
		light_transform->LookAt(
			Engine::Get()->GetScene()->GetCamera()->cameraForward() + light_transform->position,
			Engine::Get()->GetScene()->GetCamera()->cameraUp()
		);
	}*/

	if (input->keyDown(Key::F))
	{
		auto* settings = Engine::Get()->GetSceneRenderer()->GetEnvironmentSettings();
		settings->directional_light->transform.position = Engine::Get()->GetScene()->GetCamera()->cameraPosition();
		settings->directional_light->zNear = 10;
		settings->directional_light->transform.LookAt(
			Engine::Get()->GetScene()->GetCamera()->cameraForward() + settings->directional_light->transform.position,
			Engine::Get()->GetScene()->GetCamera()->cameraUp()
		);
	}
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
	return SimpleFilterShader;
}

physx::PxVec3 Game::GetGravity()
{
	return physx::PxVec3(0, 0, 0);
}

void Game::cleanup()
{
}