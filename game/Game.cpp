#include "Game.h"
#include "CommonIncludes.h"
#include "Engine.h"
#include "camera/ViewerCamera.h"
#include "loader/ModelLoader.h"
#include "loader/HierarchyLoader.h"
#include "system/Input.h"
#include "utils/MeshGeneration.h"
#include "ecs/components/MeshRenderer.h"
#include "ecs/components/MultiMeshRenderer.h"
#include "ecs/components/Static.h"
#include "scene/Scene.h"
#include "objects/LightObject.h"
#include "objects/Camera.h"
#include "render/debug/DebugDraw.h"
#include "render/texture/Texture.h"
#include "resources/TextureResource.h"
#include "resources/MultiMesh.h"
#include "resources/PhysCollider.h"
#include "resources/EntityResource.h"
#include "system/JobSystem.h"
#include "system/Input.h"
#include "entities/vehicle/VehicleUtils.h"
#include "components/PlayerInput.h"
#include "systems/ControlsSystem.h"

Game::Game() = default;
Game::~Game() = default;

using namespace System;
using namespace ECS;
using namespace ECS::systems;
using namespace physx;
using namespace Vehicle::Utils;

class TestEntity {};

struct Game::PhysicsInitializer
{
	enum class Shape
	{
		Sphere,
		Box,
		Plane
	};

	PhysicsInitializer(vec3 position, quat rotation, Shape shape, float size = 1, bool is_static = false, uint32_t word0 = COLLISION_FLAG_OBSTACLE, uint32_t word1 = COLLISION_FLAG_OBSTACLE_AGAINST)
		: position(position), rotation(rotation), shape(shape), size(size), is_static(is_static), word0(word0), word1(word1)
	{}

	uint32_t word0;
	uint32_t word1;
	bool is_static;
	vec3 position;
	quat rotation;
	float size; 
	Shape shape;// radius or half_size
};

EntityID Game::CreateMeshEntity(vec3 position, EntityID parent, Mesh* mesh)
{
	auto entity = manager->CreateEntity();
	if (parent)
		graph->AddChild(parent, entity);

	auto* transform = manager->AddComponent<components::Transform>(entity);
	*transform = components::Transform();
	transform->position = position;
	transform->bounds = mesh->aabb();

	auto* mesh_renderer = manager->AddComponent<components::MeshRenderer>(entity);
	mesh_renderer->render_queue = RenderQueue::Opaque;
	mesh_renderer->mesh = mesh;
	mesh_renderer->material = material_default;

	return entity;
}

EntityID Game::CreateMultiMeshEntity(vec3 position, quat rotation, ECS::EntityID parent, const Resources::Handle<Resources::MultiMesh>& mesh, const render::MaterialList::Handle& materials)
{
	auto entity = manager->CreateEntity();
	if (parent)
		graph->AddChild(parent, entity);

	auto* transform = manager->AddComponent<components::Transform>(entity);
	*transform = components::Transform();
	transform->position = position;

	auto* mesh_renderer = manager->AddComponent<components::MultiMeshRenderer>(entity);
	mesh_renderer->render_queue = RenderQueue::Opaque;
	mesh_renderer->multi_mesh = mesh;
	mesh_renderer->materials = materials;

	return entity;
}

EntityID Game::CreateLightEntity(vec3 position, float radius, components::Light::Type type, vec3 color)
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

PxRigidActor* Game::AddPhysics(ECS::EntityID entity, const PhysicsInitializer& init)
{
	PxRigidActor* result = nullptr;
	auto* physics = Engine::Get()->GetScene()->GetPhysics();
	auto bounds = AABB(vec3(-1), vec3(1));
	vec3 scale(init.size);

	if (init.is_static)
	{
		auto component = manager->AddComponent<components::RigidbodyStatic>(entity);
		switch (init.shape)
		{
		case PhysicsInitializer::Shape::Plane:
		{
			component->body = physics->CreatePlaneStatic(init.position, init.rotation);
			break;
		}

		default:
			throw std::runtime_error("unsupported physics parameters");
		}

		result = component->body.get();
	}
	else
	{
		auto component = manager->AddComponent<components::RigidbodyDynamic>(entity);
		switch (init.shape)
		{
		case PhysicsInitializer::Shape::Box:
		{
			component->body = physics->CreateBoxDynamic(init.position, init.rotation, init.size);
			break;
		}

		case PhysicsInitializer::Shape::Sphere:
		{
			component->body = physics->CreateSphereDynamic(init.position, init.rotation, init.size);
			break;
		}

		default:
			throw std::runtime_error("unsupported physics parameters");
		}

		result = component->body.get();
	}

	auto* transform = manager->GetComponent<components::Transform>(entity);
	if (transform)
	{
		transform->position = init.position;
		transform->rotation = init.rotation;
		transform->scale = scale;
	}

	auto filter_data = PxFilterData(init.word0, init.word1, 0, 0);

	for (int i = 0; i < result->getNbShapes(); i++)
	{
		PxShape* shape;
		result->getShapes(&shape, 1, i);
		shape->setQueryFilterData(filter_data);
		shape->setSimulationFilterData(filter_data);
	}

	return result;
}

std::vector<ECS::EntityID> Game::CreateStack(vec3 position, quat rotation, float half_extent, uint32_t count)
{
	auto* physics = Engine::Get()->GetScene()->GetPhysics();

	PxTransform t = Physics::ConvertTransform(position, rotation);

	std::vector<ECS::EntityID> result;

	for(PxU32 i=0; i<count;i++)
	{
		for(PxU32 j=0;j<count-i;j++)
		{
			PxTransform localTm(PxVec3(PxReal(j*2) - PxReal(count-i), PxReal(i*2+1), 0) * half_extent);
			Physics::ConvertTransform(t.transform(localTm), position, rotation);
			ECS::EntityID cube = CreateMeshEntity(vec3(0, 0, 0), 0, box_mesh.get());
			AddPhysics(cube, PhysicsInitializer(position, rotation, PhysicsInitializer::Shape::Box, half_extent));
			auto renderer = manager->GetComponent<components::MeshRenderer>(cube);
			renderer->material = test_material->Get();
			//renderer->material->SetColor(vec4(0.2, 0.9, 0.1, 1));
			result.push_back(cube);
		}
	}

	return result;
}

void Game::init()
{
	//ModelBundleHandle model(L"assets/resources/models/sphere.mdl");
	//auto& model_bundle = *model;

	OPTICK_EVENT();

	auto physics = Engine::Get()->GetScene()->GetPhysics();
	
	auto tire_material = Physics::Handle(physics->GetPhysX()->createMaterial(0.5, 0.5, 0.5));
	vehicle_data_cache = std::make_unique<VehicleDataCache>(*physics->GetScene(), *physics->GetAllocator(), *tire_material);

	delta_time = std::make_unique<components::DeltaTime>();

	entity_resource = Resources::EntityResource::Handle(L"assets/Entities/Sphere/sphere.entity");
	test_material = Resources::MaterialResource::Handle(L"assets/Entities/Sphere/sphere.mat");

	entity_resource_id = entity_resource->Spawn(vec3(10, -10, 10));

	sphere_bundle = ModelBundleHandle(L"assets/resources/models/sphere.mdl");
	environment = Resources::TextureResource::Handle(L"assets/resources/environment/skybox.ktx");
	lama_tex = Resources::TextureResource::Handle(L"assets/resources/lama.ktx");

	test_mesh = Resources::MultiMesh::Handle(L"assets/Models/Turret/Turret.mesh");
	player_collider = Resources::PhysCollider::Handle(L"assets/vehicle/player/car_body_collider.phys");

	auto* engine = Engine::Get();
	manager = engine->GetEntityManager();
	graph = engine->GetTransformGraph();

	manager->AddStaticComponent(vehicle_data_cache.get());
	manager->AddStaticComponent(graph);
	manager->AddStaticComponent(physics);
	manager->AddStaticComponent(delta_time.get());

	auto light_entity = CreateLightEntity(vec3(-5, 10, -10), 100, components::Light::Type::Point, vec3(1,1,1) * 2.0f);

	/*light = CreateGameObject<LightObject>();
	light->castShadows(false);
	light->color(vec3(1, 1, 1));
	light->transform()->position(vec3(50, 0, 0));
	light->radius(100);*/


	camera = std::make_unique<ViewerCamera>();

	plane_mesh = Mesh::Create();
	MeshGeneration::generateQuad(plane_mesh.get(), vec2(500, 500));
	plane_mesh->calculateNormals();
	plane_mesh->createBuffer();

	//sphere_mesh = sphere_bundle->getMesh("sphere-lib");

	sphere_mesh = Mesh::Create();
	MeshGeneration::generateSphere(sphere_mesh.get(), 50, 50, 1);
	//MeshGeneration::generateBox(sphere_mesh.get(), 1, 1, 1);
	sphere_mesh->calculateNormals();
	sphere_mesh->createBuffer();

	box_mesh = Mesh::Create();
	MeshGeneration::generateBox(box_mesh.get(), 2, 2, 2);
	box_mesh->calculateNormals();
	box_mesh->createBuffer();

	material_light_only = Material::Create();
	material_light_only->LightingEnabled(true);
	material_no_light = Material::Create();
	material_no_light->LightingEnabled(false);
	material_default = Material::Create();
	material_default->LightingEnabled(true);
	material_default->Texture0(lama_tex->Get());

	plane = CreateMeshEntity(vec3(0, 0, 0), 0, plane_mesh.get());
	auto mesh_renderer = manager->GetComponent<components::MeshRenderer>(plane);
	manager->GetComponent<components::Transform>(plane)->rotation = glm::angleAxis((float)M_PI / 2, vec3(1, 0, 0));
	mesh_renderer->material = material_light_only;
	mesh_renderer->mesh = plane_mesh.get(); 
	/*
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

	auto material = Material::Create();
	material->LightingEnabled(true);
	//material.SetRoughness(0.05 + (i / (sphere_count - 1.0f) * 0.95));
	material->SetRoughness(0.5);
	material->SetMetalness(0.05 + (i / (sphere_count - 1.0f) * 0.95));
	mesh_renderer->material = material;
	mesh_renderer->mesh = sphere_mesh.get();
	temp_entities.push_back(sphere);
	}
	*/

	auto materials = render::MaterialList::Create();
	auto material = Material::Create();
	material->LightingEnabled(true);
	materials->push_back(material);
	test_mesh_entity = CreateMultiMeshEntity(vec3(3, 10, 0), quat(), 0, test_mesh, materials);

	PxFilterData groundPlaneSimFilterData(Vehicle::Utils::COLLISION_FLAG_GROUND, Vehicle::Utils::COLLISION_FLAG_GROUND_AGAINST, 0, 0);
	auto plane_px_material = Physics::Handle(physics->GetPhysX()->createMaterial(0.5, 0.5, 0.5));
	Vehicle::Utils::CreateDrivablePlane(*manager, *physics, groundPlaneSimFilterData, plane_px_material.get());
	//physics_static.emplace_back(physics->CreatePlaneStatic(vec3(), glm::rotate(quat(), (float)M_PI / 2, vec3(0,0,1))));

	//Create a vehicle that will drive on the plane.
	Vehicle::Utils::VehicleDesc vehicleDesc = Vehicle::Utils::InitVehicleDesc(plane_px_material.get());
	vehicleDesc.chassis_collider = player_collider;
	auto vehicle_entity = Vehicle::Utils::CreateVehicle(*manager, *physics, vec3(20, 0, 0), quat(), vehicleDesc);
	manager->AddComponent<components::PlayerInput>(vehicle_entity);

	CreateStack(vec3(0), quat(), 0.5, 10);
}

void Game::UpdatePhysics(float dt)
{
	manager->GetStaticComponent<components::DeltaTime>()->physics_dt = dt;
	systems::VehicleControlSystem(*manager).ProcessChunks(manager->GetChunkListsWithComponents<components::PlayerInput, components::Vehicle>());
}

void Game::update(float dt)
{
	manager->GetStaticComponent<components::DeltaTime>()->dt = dt;
	Resources::Cache::Get().GCCollect();
	camera->Update(dt);
	auto scene_camera = Engine::Get()->GetScene()->GetCamera();
	std::vector<int> results;
	std::mutex mutex;
	auto entities = manager->GetChunkListsWithComponents<TestEntity, components::Transform>();
	ECS::CallbackSystem([dt](ECS::Chunk* chunk){
		ECS::ComponentFetcher<components::Transform> transform_fetcher(*chunk);

		for (int i = 0; i < chunk->GetEntityCount(); i++)
		{
			auto* transform = transform_fetcher.GetComponent(i);
			transform->Rotate(vec3(0, 1, 0), dt * RAD(90));
		}
	}, *manager).ProcessChunks(entities);

	// Update input
	ECS::systems::ControlsSystem(*manager).ProcessChunks(manager->GetChunkListsWithComponents<components::PlayerInput, components::Vehicle>());

	auto input = Engine::Get()->GetInput();
	if (input->keyDown(Key::Space))
	{
		auto box = CreateMeshEntity(vec3(0, 0, 0), 0, box_mesh.get());
		AddPhysics(box, PhysicsInitializer(scene_camera->Transform().position, scene_camera->Transform().rotation, PhysicsInitializer::Shape::Box, 2.0f));
		auto renderer = manager->GetComponent<components::MeshRenderer>(box);
		renderer->material = renderer->material->Clone();
		renderer->material->SetColor(vec4(Random(), Random(), Random(), 1));

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	if (input->keyDown(Key::MouseRight))
	{
		auto sphere = CreateMeshEntity(vec3(0, 0, 0), 0, sphere_mesh.get());
		auto sphere_body = static_cast<PxRigidDynamic*>(AddPhysics(sphere, PhysicsInitializer(scene_camera->Transform().position, scene_camera->Transform().rotation, PhysicsInitializer::Shape::Sphere, 1.0f)));
		sphere_body->setLinearVelocity(Physics::Convert((scene_camera->Transform().Forward() * 100.0f)));

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

IGamePhysicsDelegate* Game::GetPhysicsDelegate()
{
	return this;
}

physx::PxSimulationFilterShader Game::GetFilterShader()
{
	return VehicleFilterShader;
}

void Game::cleanup()
{
	vehicle_data_cache = nullptr;
}