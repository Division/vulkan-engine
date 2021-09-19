#include "Skinning.h"
#include "Engine.h"
#include "camera/ViewerCamera.h"
#include "system/Input.h"
#include "ecs/components/Static.h"
#include "ecs/components/AnimationController.h"
#include "ecs/components/BoneAttachment.h"
#include "ecs/components/MultiMeshRenderer.h"
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
	light->cast_shadows = true;

	return entity;
}

void Game::init()
{
	OPTICK_EVENT();

	auto* engine = Engine::Get();
	manager = engine->GetEntityManager();
	graph = engine->GetTransformGraph();

	engine->GetSceneRenderer()->SetIrradianceCubemap(Resources::TextureResource::Handle(L"assets/Textures/environment/IBL/irradiance3.ktx"));
	engine->GetSceneRenderer()->SetRadianceCubemap(Resources::TextureResource::Handle(L"assets/Textures/environment/IBL/radiance3.ktx"));

	manager->AddStaticComponent(graph);

	point_light_id = CreateLight(vec3(2.5, 4, 0), 10, ECS::components::Light::Type::Point, vec3(1, 1, 1) * 10.0f);

	camera = std::make_unique<ViewerCamera>();

	animated_entity = Resources::EntityResource::Handle(L"assets/Entities/Insect/insect.entity");
	animation = Resources::SkeletalAnimationResource::Handle(L"assets/Entities/Insect/animations/Insect@Flying.anim");
	//auto plane_handle = Resources::EntityResource::Handle(L"assets/Entities/Basic/Ground/cracks/plane10_ground_cracks.entity");
	auto plane_handle = Resources::EntityResource::Handle(L"assets/Entities/Basic/Ground/stylized/plane10_soil_water.entity");
	plane_handle->Spawn(vec3(0));

	auto scifi_box_handle = Resources::EntityResource::Handle(L"assets/Entities/Basic/cube.entity");
	auto scifi_box_id = scifi_box_handle->Spawn(vec3(0, 0, 0));
	auto box_transform = manager->GetComponent<components::Transform>(scifi_box_id);
	box_transform->scale = vec3(0.2);
	box_transform->position = vec3(-0.3, 0, 0);
	auto box_renderer = manager->GetComponent<components::MultiMeshRenderer>(scifi_box_id);
	box_renderer->material_resources = nullptr;
	box_renderer->materials = render::MaterialList::Create();
	box_renderer->materials->push_back(Material::Create());
	box_renderer->materials->at(0)->SetColor(vec4(32, 2.25, 1.3125, 1.0f));
	box_renderer->materials->at(0)->LightingEnabled(false);
	//manager->GetComponent<components::Transform>(scifi_box_id)->rotation = glm::angleAxis((float)M_PI * 4.8f, vec3(0, 1, 0)) * glm::angleAxis((float)M_PI * 4.8f, vec3(1, 0, 0));

	//auto sphere_mirror_handle = Resources::EntityResource::Handle(L"assets/Entities/Basic/Spheres/sphere_mirror.entity");
	//auto sphere_mirror_handle = Resources::EntityResource::Handle(L"assets/Entities/Basic/Spheres/sphere_rust_coated.entity");
	//sphere_mirror_handle->Spawn(vec3(4, 2, 1));

	animated_entity_id = animated_entity->Spawn(vec3(0, 0, 0));
	auto* controller = manager->GetComponent<components::AnimationController>(animated_entity_id);
	auto anim_instance = controller->mixer->PlayAnimation(animation, SkeletalAnimation::PlaybackParams().Playback(SkeletalAnimation::PlaybackMode::Loop));

	animated_entity_id = animated_entity->Spawn(vec3(5, 0, 0));
	controller = manager->GetComponent<components::AnimationController>(animated_entity_id);
	auto anim_handle = controller->mixer->PlayAnimation(animation, SkeletalAnimation::PlaybackParams().Playback(SkeletalAnimation::PlaybackMode::Loop));
	anim_handle.SetSpeed(0.06f);

	auto bone_attachment = manager->AddComponent<components::BoneAttachment>(scifi_box_id);
	bone_attachment->entity_id = animated_entity_id;
	bone_attachment->joint_index = controller->mixer->GetSkeleton()->GetJointIndex("Wing2_L");
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

	auto input = Engine::Get()->GetInput();
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