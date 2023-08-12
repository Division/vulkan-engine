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


	auto* engine = Engine::Get();
	manager = engine->GetEntityManager();

	camera = std::make_unique<ViewerCamera>();
	Engine::Get()->GetScene()->GetCamera()->Transform().position = vec3(0, 2, -4);
}

void Game::OnRender(const render::RenderCallbackData& data, render::graph::RenderGraph& graph)
{
}

void Game::update(float dt)
{
	camera->Update(dt);
}
