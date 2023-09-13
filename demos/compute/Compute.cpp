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
#include "render/renderer/SceneBuffers.h"
#include "render/buffer/GPUBuffer.h"
#include "render/device/VulkanRenderState.h"
#include "imgui/imgui.h"
#include "rps/runtime/vk/rps_vk_runtime.h"

#include "render/device/Device.h"
#include "modules/blur/Blur.h"
#include "modules/bloom/Bloom.h"

RPS_DECLARE_RPSL_ENTRY(test_triangle, main);
   
using namespace System;
using namespace ECS;
using namespace ECS::systems;
using namespace physx;
using namespace Device;

struct Game::Data
{
	render::RpsGraphHandle rpsRenderGraph;
	std::unique_ptr<Modules::Blur> blur;
	std::unique_ptr<Modules::Bloom> bloom;
	Modules::BloomSettings bloomSettings;
};

namespace
{
	constexpr uint32_t MAX_RESOLUTION = 200;
}
   
Game::Game() = default;
Game::~Game()
{
}
  
void Game::init()
{
	OPTICK_EVENT();

	auto* engine = Engine::Get();
	manager = engine->GetEntityManager();
     
	data = std::make_unique<Data>();
	data->blur = std::make_unique<Modules::Blur>("resources/compute/");
	data->bloom = std::make_unique<Modules::Bloom>("resources/compute/", &data->bloomSettings);

	camera = std::make_unique<ViewerCamera>();
	Engine::Get()->GetScene()->GetCamera()->Transform().position = vec3(0, 2, -4);

	RpsRenderGraphCreateInfo renderGraphInfo            = {};
	renderGraphInfo.mainEntryCreateInfo.hRpslEntryPoint = RPS_ENTRY_REF(test_triangle, main);

	data->rpsRenderGraph = render::RpsGraphHandle(renderGraphInfo);

	RpsResult result;
	result = rpsProgramBindNode(data->rpsRenderGraph.GetMainEntry(), "Triangle", &Game::DrawTriangle, this);
	assert(result == RPS_OK);
	result = rpsProgramBindNodeSubprogram(data->rpsRenderGraph.GetMainEntry(), "Blur", data->blur->GetSubprogram());
	assert(result == RPS_OK);
	result = rpsProgramBindNodeSubprogram(data->rpsRenderGraph.GetMainEntry(), "Bloom", data->bloom->GetSubprogram());
	assert(result == RPS_OK);
}

void Game::DrawTriangle(const RpsCmdCallbackContext* pContext)
{
	auto info = ShaderProgramInfo()
		.AddShader(ShaderProgram::Stage::Vertex, L"shaders/postprocess/postprocess.hlsl", "vs_main")
		.AddShader(ShaderProgram::Stage::Fragment, L"shaders/postprocess/postprocess.hlsl", "ps_main");
	auto shader = Engine::Get()->GetShaderCache()->GetShaderProgram(info);


	RenderMode mode;
	mode.SetDepthWriteEnabled(false);
	mode.SetDepthTestEnabled(false);
	mode.SetPolygonMode(PolygonMode::Fill);
	mode.SetPrimitiveTopology(PrimitiveTopology::TriangleList);

	ResourceBindings resource_bindings;

	const auto* descriptor_set_layout = shader->GetDescriptorSetLayout(0);
	const DescriptorSetBindings bindings(resource_bindings, *descriptor_set_layout);

	auto& state = *static_cast<VulkanRenderState*>(pContext->pUserRecordContext);

	VkRenderPass renderPassFromRps = {};
	rpsVKGetCmdRenderPass(pContext, &renderPassFromRps);
	auto renderPass = Device::VulkanRenderPass(Device::VulkanRenderPassInitializer(renderPassFromRps));

	uint32_t num = 0;
	ConstantBindings constants;
	constants.AddUIntBinding(&num, "exposure");

	auto full_screen_quad_mesh = Engine::Get()->GetSceneRenderer()->GetRendererResources().full_screen_quad_mesh.get();

	static VulkanPipeline pipeline(VulkanPipelineInitializer(shader, &renderPass, &full_screen_quad_mesh->GetVertexLayout(), &mode));
	state.BindPipeline(pipeline);
	state.SetDescriptorSetBindings(bindings, constants);
	state.Draw(*full_screen_quad_mesh->vertexBuffer(), full_screen_quad_mesh->indexCount(), 0);

	auto* scene_buffers = Engine::Get()->GetSceneRenderer()->GetSceneBuffers();
	scene_buffers->GetConstantBuffer()->Upload();
	scene_buffers->GetSkinningMatricesBuffer()->Upload();
}

void Game::update(float dt)
{
	camera->Update(dt);
}


void Game::render()
{
	auto swapchain = Engine::GetVulkanContext()->GetSwapchain();

	RpsRuntimeResource backBufferResources[16] = {};

	for (uint32_t i = 0; i < swapchain->GetImages().size(); i++)
	{
		backBufferResources[i] = { swapchain->GetImages()[i] };
	}
	const RpsRuntimeResource* argResources[] = { backBufferResources };

	RpsResourceDesc backBufferDesc = {};
	backBufferDesc.type = RPS_RESOURCE_TYPE_IMAGE_2D;
	backBufferDesc.temporalLayers = uint32_t(swapchain->GetImages().size());
	backBufferDesc.image.arrayLayers = 1;
	backBufferDesc.image.mipLevels = 1;
	backBufferDesc.image.format = rpsFormatFromVK((VkFormat)swapchain->GetImageFormat());
	backBufferDesc.image.width = swapchain->GetWidth();
	backBufferDesc.image.height = swapchain->GetHeight();
	backBufferDesc.image.sampleCount = 1;

	const float sigma = 4;
	const RpsConstant argData[] = { &backBufferDesc, &sigma };

	Engine::Get()->GetSceneRenderer()->RenderSceneGraph(*data->rpsRenderGraph, argData, argResources);
}

