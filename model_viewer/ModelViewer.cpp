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
#include "resources/TextureResource.h"
#include "system/JobSystem.h"

ModelViewer::ModelViewer() = default;
ModelViewer::~ModelViewer() = default;

using namespace System;
using namespace ECS;
using namespace ECS::systems;

class TestEntity {};

EntityID ModelViewer::CreateMeshEntity(vec3 position, EntityID parent, Mesh* mesh)
{
	auto entity = manager->CreateEntity();
	if (parent)
		graph->AddChild(parent, entity);
	
	auto* transform = manager->AddComponent<components::Transform>(entity);
	*transform = components::Transform();
	transform->position = position;

	auto* mesh_renderer = manager->AddComponent<components::MeshRenderer>(entity);
	mesh_renderer->render_queue = RenderQueue::Opaque;
	mesh_renderer->mesh = mesh;
	mesh_renderer->material_id = Engine::Get()->GetMaterialManager()->GetMaterialID(*material_default);

	return entity;
}

EntityID ModelViewer::CreateLightEntity(vec3 position, float radius, components::Light::Type type, vec3 color)
{
	auto entity = manager->CreateEntity();

	auto* transform = manager->AddComponent<components::Transform>(entity);
	transform->position = position;

	auto* light = manager->AddComponent<components::Light>(entity);
	light->type = type;
	light->color = color;
	light->radius = radius;

	return entity;
}

void ModelViewer::init()
{
	//ModelBundleHandle model(L"resources/models/sphere.mdl");
	//auto& model_bundle = *model;

	OPTICK_EVENT();

	sphere_bundle = ModelBundleHandle(L"resources/models/sphere.mdl");
	environment = Resources::TextureResource::Handle(L"resources/environment/skybox.ktx");
	lama_tex = Resources::TextureResource::Handle(L"resources/lama.ktx");

	auto* engine = Engine::Get();
	manager = engine->GetEntityManager();
	graph = engine->GetTransformGraph();

	auto light_entity = CreateLightEntity(vec3(-5, 0, -10), 100, components::Light::Type::Point, vec3(1,1,1) * 2.0f);

	/*light = CreateGameObject<LightObject>();
	light->castShadows(false);
	light->color(vec3(1, 1, 1));
	light->transform()->position(vec3(50, 0, 0));
	light->radius(100);*/


	camera = std::make_unique<ViewerCamera>();
	
	plane_mesh = Mesh::Create();
	MeshGeneration::generateQuad(plane_mesh.get(), vec2(50, 50));
	plane_mesh->calculateNormals();
	plane_mesh->createBuffer();

	//sphere_mesh = sphere_bundle->getMesh("sphere-lib");
	
	sphere_mesh = Mesh::Create();
	MeshGeneration::generateSphere(sphere_mesh.get(), 50, 50, 1);
	//MeshGeneration::generateBox(sphere_mesh.get(), 1, 1, 1);
	sphere_mesh->calculateNormals();
	sphere_mesh->createBuffer();
	

	material_light_only = std::make_shared<Material>();
	material_light_only->LightingEnabled(true);
	material_no_light = std::make_shared<Material>();
	material_no_light->LightingEnabled(false);
	material_default = std::make_shared<Material>();
	material_default->LightingEnabled(true);
	//material_default->Texture0(lama_tex->Get());
	auto* material_manager = Engine::Get()->GetMaterialManager();

	/*plane = CreateMeshEntity(vec3(0, -5, 0), 0, plane_mesh.get());
	auto mesh_renderer = manager->GetComponent<components::MeshRenderer>(plane);
	manager->GetComponent<components::Transform>(plane)->rotation = glm::angleAxis((float)M_PI / 2, vec3(1, 0, 0));
	mesh_renderer->material_id = material_manager->GetMaterialID(*material_light_only);
	mesh_renderer->mesh = plane_mesh.get(); */

	uint32_t sphere_count = 10;
	uint32_t sphere_offset = 3;
	float start_pos = sphere_offset * (sphere_count - 1) / 2.0f;
	for (int i = 0; i < sphere_count; i++)
	{
		auto sphere = CreateMeshEntity(vec3(0, -5, 0), 0, sphere_mesh.get());
		manager->AddComponent<TestEntity>(sphere);
		auto mesh_renderer = manager->GetComponent<components::MeshRenderer>(sphere);
		manager->GetComponent<components::Transform>(sphere)->position = vec3(i * sphere_offset, 0, -10);
		manager->GetComponent<components::Transform>(sphere)->scale = vec3(1, 1, 1);

		Material material;
		material.LightingEnabled(true);
		//material.SetRoughness(0.05 + (i / (sphere_count - 1.0f) * 0.95));
		material.SetRoughness(0.5);
		material.SetMetalness(0.05 + (i / (sphere_count - 1.0f) * 0.95));
		mesh_renderer->material_id = material_manager->GetMaterialID(material);
		mesh_renderer->mesh = sphere_mesh.get();
	}

}

void ModelViewer::update(float dt)
{
	Resources::Cache::Get().GCCollect();
	camera->Update(dt);

	std::vector<int> results;
	std::mutex mutex;
	Engine::Get()->GetDebugDraw()->DrawAABB(vec3(0,0,0), vec3(10,10,10), vec4(1, 0, 0, 1));
	Engine::Get()->GetDebugDraw()->DrawPoint(vec3(), vec4(1, 1, 1, 10));
	///*
	auto entities = manager->GetChunkListsWithComponents<TestEntity, components::Transform>();
	ECS::CallbackSystem([dt](ECS::Chunk* chunk){
		ECS::ComponentFetcher<components::Transform> transform_fetcher(*chunk);

		for (int i = 0; i < chunk->GetEntityCount(); i++)
		{
			auto* transform = transform_fetcher.GetComponent(i);
			transform->Rotate(vec3(0, 1, 0), dt * RAD(90));
		}
	}, *manager).ProcessChunks(entities);

	//*/

	/*
	for (int i = 0; i < 100; i++)
	{
		OPTICK_EVENT("Spawning job");

		Thread::Scheduler::Get().SpawnJob<Thread::FunctionJob>(Thread::Job::Priority::High, [&mutex, &results, i] {
			OPTICK_EVENT("job...");
			std::this_thread::sleep_for(std::chrono::microseconds(100));
			int result = i * i;
			std::scoped_lock lock(mutex);
			results.push_back(result);
		});
	}

	Thread::Scheduler::Get().Wait(Thread::Job::Priority::High);
	{
		OPTICK_EVENT("OUTPUT");
		std::stringstream s;
		for (auto i : results)
			s << i << ",";
		s << "\n";

		std::cout << s.str();

	}*/
}

void ModelViewer::cleanup()
{
}