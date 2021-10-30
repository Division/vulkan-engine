#include "Compute.h"
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
#include "render/renderer/RenderGraph.h"
#include "render/buffer/GPUBuffer.h"
#include "render/device/VulkanRenderState.h"
#include "imgui/imgui.h"

using namespace System;
using namespace ECS;
using namespace ECS::systems;
using namespace physx;

struct Game::Handles
{
	render::SceneRenderer::RenderDispatcher::Handle render_callback;
};

namespace
{
	constexpr uint32_t MAX_RESOLUTION = 200;
}

Game::Game() = default;
Game::~Game() = default;

void Game::init()
{
	OPTICK_EVENT();

	handles = std::make_unique<Handles>();

	auto* engine = Engine::Get();
	manager = engine->GetEntityManager();

	camera = std::make_unique<ViewerCamera>();
	Engine::Get()->GetScene()->GetCamera()->Transform().position = vec3(0, 2, -4);

	auto scifi_box_handle = Resources::EntityResource::Handle(L"assets/Entities/Basic/cube.entity");
	box_id = scifi_box_handle->Spawn(vec3(0, 0, 0));
	auto box_transform = manager->GetComponent<components::Transform>(box_id);
	box_transform->scale = vec3(0.1f);
	box_transform->bounds = AABB::Infinity();
	auto box_renderer = manager->GetComponent<components::MultiMeshRenderer>(box_id);
	box_renderer->material_resources = nullptr;
	box_renderer->materials = render::MaterialList::Create();
	box_renderer->materials->push_back(Material::Create());
	box_renderer->materials->at(0)->LightingEnabled(false);
	box_renderer->materials->at(0)->SetShaderPath(L"assets/compute/compute_instances.hlsl");

	resolution = 100;

	box_renderer->instance_count = resolution * resolution;

	handles->render_callback = Engine::Get()->GetSceneRenderer()->AddRenderCallback(std::bind(&Game::OnRender, this, std::placeholders::_1, std::placeholders::_2));
	compute_buffer = std::make_unique<Device::GPUBuffer>("compute", sizeof(vec4) * MAX_RESOLUTION * MAX_RESOLUTION, Device::BufferType::Storage);

	compute_program = Engine::Get()->GetShaderCache()->GetShaderProgram(Device::ShaderProgramInfo().AddShader(
		Device::ShaderProgramInfo::ShaderData(Device::ShaderProgram::Stage::Compute, L"assets/compute/compute_shader.hlsl", "FunctionKernel"))
	);

	box_renderer->materials->at(0)->AddExtraBuffer(compute_buffer->GetBuffer(), "positions");
}

void Game::OnRender(const render::RenderCallbackData& data, render::graph::RenderGraph& graph)
{
	struct PassInfo
	{
		render::graph::DependencyNode* compute_output = nullptr;
	};

	auto* compute_buffer_wrapper = graph.RegisterBuffer(*compute_buffer->GetBuffer().get());
	auto compute_pass_info = graph.AddPass<PassInfo>("compute pass", render::profiler::ProfilerName::PassCompute, [&](render::graph::IRenderPassBuilder& builder)
	{
		builder.SetCompute(false);

		PassInfo result;
		result.compute_output = builder.AddOutput(*compute_buffer_wrapper);
		return result;
	}, [&](Device::VulkanRenderState& state)
	{
		Device::ResourceBindings bindings; 
		Device::ConstantBindings constants;
		const float time = Engine::Get()->time();
		const float step = 2.0f / resolution;
		constants.AddFloatBinding(&time, "Time");
		constants.AddFloatBinding(&step, "Step");

		const uint resolution_uint = resolution;
		constants.AddUIntBinding(&resolution_uint, "Resolution");
		bindings.AddBufferBinding("positions", compute_buffer->GetBuffer().get(), compute_buffer->GetSize());

		const uint32_t threads = ceilf(resolution / 8.0f);
		state.Dispatch(*compute_program, bindings, constants, uvec3(threads, threads, 1));
	});

	data.scene_renderer.AddUserRenderDependency(render::SceneRenderer::RenderDependencyType::DepthPrepass, *compute_pass_info.compute_output);
}

void Game::update(float dt)
{
	camera->Update(dt);

	ImGui::Begin("Demo");
	ImGui::SliderInt("Resolution", &resolution, 0, MAX_RESOLUTION);
	ImGui::End();

	auto box_renderer = manager->GetComponent<components::MultiMeshRenderer>(box_id);
	const auto new_instance_count = resolution * resolution;
	if (new_instance_count != box_renderer->instance_count)
	{
		box_renderer->instance_count = new_instance_count;
		box_renderer->draw_calls.Reset();
	}
}
