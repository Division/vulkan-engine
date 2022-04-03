#include "ParticleSystem.h"
#include "Engine.h"
#include "camera/ViewerCamera.h"
#include "system/Input.h"
#include "ecs/components/Static.h"
#include "ecs/components/AnimationController.h"
#include "ecs/components/BoneAttachment.h"
#include "ecs/components/MultiMeshRenderer.h"
#include "ecs/components/Particles.h"
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
#include "render/renderer/RenderGraph.h"
#include "render/buffer/GPUBuffer.h"
#include "render/device/VulkanRenderState.h"
#include "imgui/imgui.h"

using namespace System;
using namespace ECS;
using namespace ECS::systems;

namespace
{
	constexpr uint32_t MAX_RESOLUTION = 200;

#pragma pack(push, 1)
	struct ParticleAttractor
	{
		vec3 position;
		float power;
	};
#pragma pack(pop)
}

struct Game::GPUResources
{
	Device::DynamicBuffer<ParticleAttractor> attractors_buffer;

	std::vector<ParticleAttractor> attractors = {
		{ { 1, 0, 0 }, 150 * 2 },
		{ { 3, 6, 0 }, 200 * 3 },
		{ { 9, 10, 0 }, -130 * 2 }
	};

	GPUResources()
		: attractors_buffer("attractors buffer", 20 * sizeof(ParticleAttractor), Device::BufferType::Storage, false)
	{
	}
};

Game::Game() = default;
Game::~Game() = default;

ECS::EntityID Game::CreateSphere(vec4 color)
{
	auto sphere_template = Resources::EntityResource::Handle(L"assets/Entities/Basic/sphere.entity");
	auto sphere_id = sphere_template->Spawn();
	
	auto material = Material::Create();
	material->LightingEnabled(false);
	material->SetColor(color);

	{
		auto renderer = manager->GetComponent<components::MultiMeshRenderer>(sphere_id);
		renderer->material_resources = nullptr;
		renderer->materials = render::MaterialList::Create();
		renderer->materials->push_back(material);
	}

	return sphere_id;
}

void Game::init()
{
	OPTICK_EVENT();

	auto* engine = Engine::Get();
	manager = engine->GetEntityManager();

	camera = std::make_unique<ViewerCamera>();
	Engine::Get()->GetScene()->GetCamera()->Transform().position = vec3(0, 2, -4);

	gpu_resources = std::make_unique<GPUResources>();

	for (uint32_t i = 0; i < gpu_resources->attractors.size(); i++)
	{
		spheres.push_back(CreateSphere(gpu_resources->attractors[i].power > 0 ? vec4(0.3, 4, 0.3, 1) : vec4(4, 0.3, 0.3, 1)));
	}

	//attractors_buffer = std::make_unique<Device::DynamicBuffer>("attractors buffer", attractors.size() * sizeof(ParticleAttractor), Device::BufferType::Storage, attractors.data());

	render::ConstantBindingStorage particle_constants;
	render::ResourceBindingStorage particle_resources;
	particle_constants.AddUIntConstant("attractor_count", gpu_resources->attractors.size());
	particle_resources.AddBuffer("attractors", gpu_resources->attractors_buffer.GetBuffer());

	particle_system_id = manager->CreateEntity();
	{
		auto transform = manager->AddComponent<components::Transform>(particle_system_id);
		transform->bounds = AABB::Infinity();
		transform->position = vec3(0, 0, 0);
	}

	auto material = Material::Create();
	material->LightingEnabled(false);
	material->SetColor(vec4(0.6, 0.4, 2.6, 0.8));
	material->SetShaderPath(L"shaders/particles/particle_material_default.hlsl");
	material->SetRenderQueue(RenderQueue::Additive);
	material->SetTexture0Resource(Resources::TextureResource::SRGB(L"assets/Textures/effects/light.dds"));

	auto emitter_initializer = components::ParticleEmitter::Initializer(1000000, material)
		.SetExtraBindings({ particle_resources, particle_constants })
		.SetEmitterGeometry(components::ParticleEmitter::EmitterGeometrySphere())
		.SetShaderPath(L"shaders/particles/attractors_particle_system.hlsl")
		.SetEmissionParams(components::ParticleEmitter::EmissionParams()
			.SetSize({ 0.02f, 0.04f })
			.SetEmissionRate(150000)
			.SetLife({ 3, 8 })
			.SetConeAngle(M_PI / 12)
			.SetSpeed({ 4, 20 })
		);
		
	manager->AddComponent<components::ParticleEmitter>(particle_system_id, emitter_initializer);
}

void Game::update(float dt)
{
	//Engine::Get()->GetDebugDraw()->DrawAxis(vec3(0), glm::quat(), 15);

	gpu_resources->attractors_buffer.Map();

	const auto time = Engine::Get()->time();

	const float radius = 5.0f;
	gpu_resources->attractors[0].position = vec3(cos(time * M_PI * 0.4f) * radius, cos(time * M_PI * 0.3f) * 4, sin(time * M_PI * 0.4f) * radius) + vec3(-radius * 1.2f, 2, 0 );

	gpu_resources->attractors[1].position.x = cos(time * M_PI * 0.25f) * 3 + 3;
	gpu_resources->attractors[2].position.y = cos(time * M_PI * 0.28f) * 6 + 4;

	uint32_t index = 0;
	for (auto& attractor : gpu_resources->attractors)
	{
		gpu_resources->attractors_buffer.Append(attractor);
		auto transform = manager->GetComponent<components::Transform>(spheres[index++]);
		transform->position = attractor.position;
		transform->scale = vec3(0.3f);
		//Engine::Get()->GetDebugDraw()->DrawPoint(attractor.position, vec3(1, 0, 0), 10);
	}

	gpu_resources->attractors_buffer.Unmap();

	camera->Update(dt);
}
