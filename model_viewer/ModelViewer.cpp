#include "ModelViewer.h"
#include "CommonIncludes.h"
#include "Engine.h"
#include "camera/ViewerCamera.h"
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

ModelViewer::ModelViewer() = default;
ModelViewer::~ModelViewer() = default;

using namespace core;
using namespace core::system;
using namespace core::ECS;
using namespace core::ECS::systems;

EntityID ModelViewer::CreateCubeEntity(vec3 position, EntityID parent, Mesh* mesh)
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
	mesh_renderer->mesh = mesh;
	mesh_renderer->material_id = core::Engine::Get()->GetMaterialManager()->GetMaterialID(*material_default);

	return entity;
}

void ModelViewer::init()
{
	auto* engine = core::Engine::Get();
	manager = engine->GetEntityManager();
	graph = engine->GetTransformGraph();

	auto light = CreateGameObject<LightObject>();
	light->castShadows(false);
	light->color(vec3(0, 0, 0));
	//lama_tex = loader::LoadTexture("resources/lama.jpg");
	environment = loader::LoadTexture("resources/environment/skybox.ktx");
	lama_tex = loader::LoadTexture("resources/lama.ktx");

	camera = std::make_unique<ViewerCamera>();
	
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
	auto* material_manager = core::Engine::Get()->GetMaterialManager();

	plane = CreateCubeEntity(vec3(0, -5, 0), 0, plane_mesh.get());
	auto mesh_renderer = manager->GetComponent<components::MeshRenderer>(plane);
	manager->GetComponent<components::Transform>(plane)->rotation = glm::angleAxis((float)M_PI / 2, vec3(1, 0, 0));
	mesh_renderer->material_id = material_manager->GetMaterialID(*material_light_only);
	mesh_renderer->mesh = plane_mesh.get();
}

void ModelViewer::update(float dt)
{
	camera->Update(dt);
}

void ModelViewer::cleanup()
{
	
}