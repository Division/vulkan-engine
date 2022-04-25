#include "islands.h"
#include "Engine.h"
#include "generation/LandMeshGenerator.h"
#include "camera/ViewerCamera.h"
#include "system/Input.h"
#include "ecs/components/Static.h"
#include "scene/Scene.h"
#include "render/debug/DebugDraw.h"
#include "system/Input.h"
#include "objects/Camera.h"
#include "utils/Math.h"
#include "render/renderer/EnvironmentSettings.h"
#include "render/renderer/SceneRenderer.h"

#include "imgui/imgui.h"

using namespace System;
using namespace ECS;
using namespace ECS::systems;

struct Game::GPUResources
{

	GPUResources(){
		
	}
};

Game::Game() = default;
Game::~Game() = default;

void Game::init()
{
	OPTICK_EVENT();

	auto* engine = Engine::Get();
	manager = engine->GetEntityManager();

	camera = std::make_unique<ViewerCamera>();
	Engine::Get()->GetScene()->GetCamera()->Transform().position = vec3(0, 2, -4);

	gpu_resources = std::make_unique<GPUResources>();

	auto* settings = Engine::Get()->GetSceneRenderer()->GetEnvironmentSettings();
	settings->directional_light->enabled = true;
	settings->directional_light->zNear = 1;
	settings->directional_light->transform.position = vec3(-10, 30, -20);
	settings->directional_light->orthographic_size = vec2(30, 20);
	settings->directional_light->cast_shadows = false;
	settings->directional_light->transform.LookAt(
		vec3(0, 0, 0),
		vec3(0, 1, 0)
	); 

	Generation::CreateLandMesh(*manager, vec3(0), L"assets/islands/heightmap/copy_1x1_island.raw");
} 

void Game::update(float dt)
{
	Engine::Get()->GetDebugDraw()->DrawAxis(vec3(0), glm::quat(), 5);

	camera->Update(dt);
}
