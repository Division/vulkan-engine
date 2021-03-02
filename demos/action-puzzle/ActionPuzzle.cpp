#include "ActionPuzzle.h"
#include "CommonIncludes.h"
#include "Engine.h"
#include "camera/ViewerCamera.h"
#include "system/Input.h"
#include "ecs/components/Static.h"
#include "scene/Scene.h"
#include "render/debug/DebugDraw.h"
#include "render/texture/Texture.h"
#include "resources/MultiMesh.h"
#include "resources/EntityResource.h"
#include "system/JobSystem.h"
#include "system/Input.h"
#include "entities/vehicle/VehicleUtils.h"
#include "components/PlayerInput.h"
#include "systems/ControlsSystem.h"
#include "gameplay/Gameplay.h"
#include "render/renderer/SceneRenderer.h"
#include "render/renderer/EnvironmentSettings.h"

Game::Game() = default;
Game::~Game() = default;

using namespace System;
using namespace ECS;
using namespace ECS::systems;
using namespace physx;
using namespace Vehicle::Utils;

void Game::init()
{
	OPTICK_EVENT();

	gameplay = std::make_unique<game::Gameplay>();

	auto physics = Engine::Get()->GetScene()->GetPhysics();
	
	auto tire_material = Physics::Handle(physics->GetPhysX()->createMaterial(0.5, 0.5, 0.5));
	vehicle_data_cache = std::make_unique<VehicleDataCache>(*physics->GetScene(), *physics->GetAllocator(), *tire_material);

	auto* engine = Engine::Get();
	manager = engine->GetEntityManager();
	graph = engine->GetTransformGraph();

	engine->GetSceneRenderer()->SetIrradianceCubemap(Resources::TextureResource::Handle(L"assets/art/Textures/environment/IBL/irradiance3.ktx"));
	engine->GetSceneRenderer()->SetRadianceCubemap(Resources::TextureResource::Handle(L"assets/art/Textures/environment/IBL/radiance3.ktx"));
	engine->GetSceneRenderer()->GetEnvironmentSettings()->environment_brightness = 0.05;
	engine->GetSceneRenderer()->GetEnvironmentSettings()->exposure = 1.2;

	manager->AddStaticComponent(vehicle_data_cache.get());
	manager->AddStaticComponent(graph);
	manager->AddStaticComponent(physics);

	camera = std::make_unique<ViewerCamera>();

	gameplay->StartGame();
}

void Game::UpdatePhysics(float dt)
{
	gameplay->UpdatePhysics(dt);
	systems::VehicleControlSystem(*manager).ProcessChunks(manager->GetChunkListsWithComponents<components::PlayerInput, components::Vehicle>());
}

void Game::update(float dt)
{
	Resources::Cache::Get().GCCollect();
	camera->Update(dt);
	gameplay->Update(dt);
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
	PX_UNUSED(attributes0);
	PX_UNUSED(attributes1);
	PX_UNUSED(constantBlock);
	PX_UNUSED(constantBlockSize);

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
	vehicle_data_cache = nullptr;
}