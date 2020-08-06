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
#include "render/debug/DebugDraw.h"
#include "render/texture/Texture.h"
#include "loader/TextureLoader.h"

Game::Game() = default;
Game::~Game() = default;

using namespace system;
using namespace ECS;
using namespace ECS::systems;

EntityID Game::CreateCubeEntity(vec3 position, EntityID parent)
{
	auto entity = manager->CreateEntity();
	if (parent)
		graph->AddChild(parent, entity);
	
	auto* transform = manager->AddComponent<components::Transform>(entity);
	*transform = components::Transform();
	transform->position = position;

	auto* mesh_renderer = manager->AddComponent<components::MeshRenderer>(entity);
	*mesh_renderer = components::MeshRenderer();
	mesh_renderer->render_queue = RenderQueue::Opaque;
	mesh_renderer->mesh = box_mesh.get();
	mesh_renderer->material_id = Engine::Get()->GetMaterialManager()->GetMaterialID(*material_default);

	return entity;
}

void Game::init()
{
	auto* engine = Engine::Get();
	manager = engine->GetEntityManager();
	graph = engine->GetTransformGraph();

	//lama_tex = loader::LoadTexture("resources/lama.jpg");
	environment = loader::LoadTexture("resources/environment/skybox.ktx");
	lama_tex = loader::LoadTexture("resources/lama.ktx");

	camera = CreateGameObject<FollowCamera>();
	camera->setFreeCamera(true);
	
	box_mesh = std::make_shared<Mesh>();
	MeshGeneration::generateBox(box_mesh.get(), 1, 1, 1);
	box_mesh->calculateNormals();
	std::vector<vec4> colors(box_mesh->vertexCount(), vec4(1, 0, 0, 1));
	box_mesh->setColors(colors.data(), colors.size());
	box_mesh->createBuffer();
	plane_mesh = std::make_shared<Mesh>();
	MeshGeneration::generateQuad(plane_mesh.get(), vec2(50, 50));
	plane_mesh->calculateNormals();
	plane_mesh->createBuffer();

	material_light_only = std::make_shared<Material>();;
	material_light_only->lightingEnabled(true);
	material_no_light = std::make_shared<Material>();
	material_no_light->lightingEnabled(false);
	material_default = std::make_shared<Material>();
	material_default->lightingEnabled(true);
	material_default->texture0(lama_tex);
	auto* material_manager = Engine::Get()->GetMaterialManager();

	entity1 = CreateCubeEntity(vec3(0, 3, 0), 0);
	entity2 = CreateCubeEntity(vec3(2, 0, 0), entity1);
	plane = CreateCubeEntity(vec3(0, -5, 0), 0);
	auto mesh_renderer = manager->GetComponent<components::MeshRenderer>(plane);
	mesh_renderer->material_id = material_manager->GetMaterialID(*material_light_only);
	mesh_renderer->mesh = plane_mesh.get();
	manager->GetComponent<components::Transform>(plane)->rotation = glm::angleAxis((float)M_PI / 2, vec3(1, 0, 0));

 	light = CreateGameObject<LightObject>();
	light->castShadows(true);
	light->color(vec3(10, 10, 10));
	light->transform()->setPosition(vec3(7, 7, 3));
	light->transform()->setRotation(glm::angleAxis(-(float)M_PI / 2, vec3(1, 0, 0)));
	light->radius(30);
	light->coneAngle(60);
	//light->type(LightObjectType::Spot);

	for (int i = 0; i < 10; i++)
		for (int j = 0; j < 10; j++)
		{
			auto entity = CreateCubeEntity(vec3(i * 2, -2, j * 2), 0);
		}

	auto light_pos_entity = CreateCubeEntity(light->transform()->position(), 0);
	manager->GetComponent<components::Transform>(light_pos_entity)->scale = vec3(0.1, 0.1, 0.1);
	manager->GetComponent<components::MeshRenderer>(light_pos_entity)->material_id = material_manager->GetMaterialID(*material_no_light);
}

void Game::update(float dt)
{
	auto* transform1 = manager->GetComponent<components::Transform>(entity1);
	transform1->Rotate(vec3(0, 0, 1), M_PI * dt);
	
	auto* debug_draw = Engine::Get()->GetDebugDraw();
	debug_draw->DrawAABB(AABB(vec3(0, 0, 0), vec3(4, 4, 4)), vec4(1, 0, 0, 1));
	debug_draw->DrawAABB(AABB(vec3(5, 5, 0), vec3(8, 8, 8)), vec4(0.5, 1, 0, 1));
	debug_draw->DrawLine(vec3(5, 5, 0), vec3(8, 8, 8), vec4(0.5, 0.5, 1, 1));

	debug_draw->DrawPoint(vec3(0, 0, 0), vec3(0, 0, 1), 6.0f);
	debug_draw->DrawPoint(vec3(4, 4, 4), vec3(0, 0, 1), 6.0f);

	auto input = Engine::Get()->GetInput();
	if (input->keyDown(system::Key::Space))
		light->transform()->setMatrix(Engine::Get()->GetScene()->GetCamera()->transform()->worldMatrix());
}

void Game::cleanup()
{
	material_default->texture0(nullptr);
}