#include "Game.h"
#include "CommonIncludes.h"
#include "Engine.h"
#include "../game/objects/FollowCamera.h"
#include "loader/ModelLoader.h"
#include "loader/HierarchyLoader.h"
#include "system/Input.h"
#include "utils/MeshGeneration.h"
#include "ecs/components/MeshRenderer.h"
#include "scene/Scene.h"
#include "objects/LightObject.h"
#include "render/material/MaterialManager.h"

Game::Game() = default;
Game::~Game() = default;

using namespace core::system;
using namespace core::ECS;
using namespace core::ECS::systems;

EntityID Game::CreateCubeEntity(vec3 position, EntityID parent)
{
	auto entity = manager->CreateEntity();
	if (parent)
		graph->AddChild(parent, entity);
	
	auto* transform = manager->AddComponent<components::Transform>(entity);
	*transform = components::Transform();
	transform->position = position;

	auto* mesh_renderer = manager->AddComponent<components::MeshRenderer>(entity);
	mesh_renderer->render_queue = RenderQueue::Opaque;
	mesh_renderer->mesh = box_mesh.get();
	mesh_renderer->material_id = core::Engine::Get()->GetMaterialManager()->GetMaterialID(*material_default);

	return entity;
}

void Game::init()
{
	auto* engine = core::Engine::Get();
	manager = engine->GetEntityManager();
	graph = engine->GetTransformGraph();

	camera = CreateGameObject<FollowCamera>();
	camera->setFreeCamera(true);
	
	box_mesh = std::make_shared<Mesh>();
	MeshGeneration::generateBox(box_mesh, 1, 1, 1);
	box_mesh->calculateNormals();
	std::vector<vec4> colors(box_mesh->vertexCount(), vec4(1, 0, 0, 1));
	box_mesh->setColors(colors.data(), colors.size());
	box_mesh->createBuffer();

	material_no_light = std::make_shared<Material>();
	material_no_light->lightingEnabled(false);
	material_default = std::make_shared<Material>();
	material_default->lightingEnabled(true);

	entity1 = CreateCubeEntity(vec3(0, 3, 0), 0);
	entity2 = CreateCubeEntity(vec3(2, 0, 0), entity1);

	auto light = CreateGameObject<LightObject>();
	light->color(vec3(1, 1, 1));
	light->transform()->setPosition(vec3(7, 7, 3));
	light->radius(30);

	for (int i = 0; i < 10; i++)
		for (int j = 0; j < 10; j++)
		{
			auto entity = CreateCubeEntity(vec3(i * 2, -2, j * 2), 0);
		}

	auto light_pos_entity = CreateCubeEntity(light->transform()->position(), 0);
	manager->GetComponent<components::Transform>(light_pos_entity)->scale = vec3(0.1, 0.1, 0.1);
	auto* material_manager = core::Engine::Get()->GetMaterialManager();
	manager->GetComponent<components::MeshRenderer>(light_pos_entity)->material_id = material_manager->GetMaterialID(*material_no_light);
}

void Game::update(float dt)
{
	auto* transform1 = manager->GetComponent<components::Transform>(entity1);
	transform1->Rotate(vec3(0, 0, 1), M_PI * dt);
}

void Game::cleanup()
{
	
}