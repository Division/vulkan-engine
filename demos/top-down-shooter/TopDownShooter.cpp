#include "TopDownShooter.h"
#include "Engine.h"
#include "camera/ViewerCamera.h"
#include "system/Input.h"
#include "ecs/components/Static.h"
#include "ecs/components/AnimationController.h"
#include "ecs/components/BoneAttachment.h"
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

	typedef components::CharacterController::MoveAnimationType MoveAnimationType;
	typedef components::CharacterController::StationaryAnimationType StationaryAnimationType;

	auto character_controller = manager->GetComponent<components::CharacterController>(player_id);
	character_controller->stationary_animations[StationaryAnimationType::Idle]		= SkeletalAnimationResource::Handle(L"assets/top-down-shooter/characters/uetest/Rifle@Idle.anim");
	character_controller->stationary_animations[StationaryAnimationType::IdleAim]	= SkeletalAnimationResource::Handle(L"assets/top-down-shooter/characters/uetest/Rifle@Idle_GunDown.anim");
	character_controller->move_animations[MoveAnimationType::RunForward]			= SkeletalAnimationResource::Handle(L"assets/top-down-shooter/characters/uetest/Rifle@RunFwdLoop.anim");
	character_controller->move_animations[MoveAnimationType::WalkForward]			= SkeletalAnimationResource::Handle(L"assets/top-down-shooter/characters/uetest/Rifle@WalkFwdLoop.anim");
	character_controller->move_animations[MoveAnimationType::RunBackward]			= SkeletalAnimationResource::Handle(L"assets/top-down-shooter/characters/uetest/Rifle@RunBwdLoop.anim");
	character_controller->move_animations[MoveAnimationType::RunLeft]				= SkeletalAnimationResource::Handle(L"assets/top-down-shooter/characters/uetest/Rifle@StrafeRunLeftLoop.anim");
	character_controller->move_animations[MoveAnimationType::RunRight]				= SkeletalAnimationResource::Handle(L"assets/top-down-shooter/characters/uetest/Rifle@StrafeRunRightLoop.anim");
	character_controller->move_animations[MoveAnimationType::RunBackwardRight]		= SkeletalAnimationResource::Handle(L"assets/top-down-shooter/characters/uetest/Rifle@StrafeRun135RightLoop.anim");
	character_controller->move_animations[MoveAnimationType::RunForwardRight]		= SkeletalAnimationResource::Handle(L"assets/top-down-shooter/characters/uetest/Rifle@StrafeRun45RightLoop.anim");
	character_controller->move_animations[MoveAnimationType::RunBackwardLeft]		= SkeletalAnimationResource::Handle(L"assets/top-down-shooter/characters/uetest/Rifle@StrafeRun135LeftLoop.anim");
	character_controller->move_animations[MoveAnimationType::RunForwardLeft]		= SkeletalAnimationResource::Handle(L"assets/top-down-shooter/characters/uetest/Rifle@StrafeRun45LeftLoop.anim");

	auto animation_controller = manager->GetComponent<components::AnimationController>(player_id);
	animation_controller->mixer->SetRootMotionEnabled(true);
	character_controller->Startup(animation_controller);

	auto rifle_handle = Resources::EntityResource::Handle(L"assets/top-down-shooter/characters/uetest/m4_rifle.entity");
	rifle_id = rifle_handle->Spawn(vec3(0, 0, 0));
	auto rifle_transform = manager->GetComponent<components::Transform>(rifle_id);
	rifle_transform->scale = vec3(1);
	rifle_transform->position = vec3(0, -0.01, 0);
	rifle_transform->rotation = glm::angleAxis(-(float)M_PI / 2, vec3(1, 0, 0));

	auto bone_attachment = manager->AddComponent<components::BoneAttachment>(rifle_id);
	bone_attachment->entity_id = player_id;
	bone_attachment->joint_index = animation_controller->mixer->GetSkeleton()->GetJointIndex("ik_hand_gun");


	return player_id;
}

void Game::init()
{
	OPTICK_EVENT();

	auto* engine = Engine::Get();
	manager = engine->GetEntityManager();
	graph = engine->GetTransformGraph();

	manager->RegisterComponentTemplate<ECS::components::CharacterControllerTemplate>("CharacterController");

	engine->GetSceneRenderer()->SetIrradianceCubemap(Resources::TextureResource::Handle(L"assets/Textures/environment/IBL/irradiance3.ktx"));
	engine->GetSceneRenderer()->SetRadianceCubemap(Resources::TextureResource::Handle(L"assets/Textures/environment/IBL/radiance3.ktx"));

	player_id = CreatePlayer();

	manager->AddStaticComponent(graph);

	point_light_id = CreateLight(vec3(2.5, 4, 0), 10, ECS::components::Light::Type::Point, vec3(1, 1, 1) * 10.0f);

	camera = std::make_unique<ViewerCamera>();

	auto plane_handle = Resources::EntityResource::Handle(L"assets/Entities/Basic/Ground/stylized/plane10_soil_water.entity");
	plane_handle->Spawn(vec3(0));
}

void Game::UpdatePhysics(float dt)
{
}

std::optional<vec3> Game::GetMouseTarget()
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

void Game::UpdatePlayer(float dt)
{
	auto input = Engine::Get()->GetInput();


	auto character_controller = manager->GetComponent<components::CharacterController>(player_id);
	auto transform = manager->GetComponent<components::Transform>(player_id);
	auto& character_input = character_controller->input;
	character_input.aim_direction = vec2(0);
	character_input.move_direction = vec2(0);
	
	if (camera_control)
		return;

	auto target = GetMouseTarget();
	if (target)
	{
		auto dir = *target - transform->position;
		float length = glm::length(dir);
		if (length > 0.001f)
		{
			dir /= length;
			character_input.aim_direction = vec2(dir.x, dir.z);
		}
	}

	if (input->keyDown(Key::Up) || input->keyDown(Key::W))
		character_input.move_direction.y = -1;
	else if (input->keyDown(Key::Down) || input->keyDown(Key::S))
		character_input.move_direction.y = 1;
	if (input->keyDown(Key::Left) || input->keyDown(Key::A))
		character_input.move_direction.x = -1;
	else if (input->keyDown(Key::Right) || input->keyDown(Key::D))
		character_input.move_direction.x = 1;

	float length = glm::length(character_input.move_direction);
	if (length > 0.001f)
		character_input.move_direction /= length;
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

	auto controllers = manager->GetChunkListsWithComponents<components::CharacterController, components::AnimationController>();
	CharacterControllerSystem(*manager).ProcessChunks(controllers);

	UpdatePlayer(dt);

	auto input = Engine::Get()->GetInput();
	if (input->FirstKeyDown(Key::Tab))
		camera_control = !camera_control;

	if (camera_control)
		camera->Update(dt);
	else
		UpdateFollowCamera();

	Engine::Get()->GetDebugDraw()->DrawLine(vec3(), vec3(1, 0, 0), vec4(1, 0, 0, 1));
	Engine::Get()->GetDebugDraw()->DrawLine(vec3(), vec3(0, 1, 0), vec4(0, 1, 0, 1));
	Engine::Get()->GetDebugDraw()->DrawLine(vec3(), vec3(0, 0, 1), vec4(0, 0, 1, 1));

	if (input->keyDown(Key::Space))
	{
		auto light_transform = manager->GetComponent<components::Transform>(point_light_id);
		light_transform->position = Engine::Get()->GetScene()->GetCamera()->cameraPosition();
		light_transform->LookAt(
			Engine::Get()->GetScene()->GetCamera()->cameraForward() + light_transform->position,
			Engine::Get()->GetScene()->GetCamera()->cameraUp()
		);
	}

	if (input->keyDown(Key::F))
	{
		auto* settings = Engine::Get()->GetSceneRenderer()->GetEnvironmentSettings();
		settings->directional_light->transform.position = Engine::Get()->GetScene()->GetCamera()->cameraPosition();
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
	return PxDefaultSimulationFilterShader;
}

physx::PxVec3 Game::GetGravity()
{
	return physx::PxVec3(0, 0, 0);
}

void Game::cleanup()
{
}