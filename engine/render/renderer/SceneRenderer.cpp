#include "SceneRenderer.h"
#include "scene/Scene.h"
#include "render/device/Device.h"
#include "CommonIncludes.h"
#include "Engine.h"
#include "loader/TextureLoader.h"
#include "render/texture/Texture.h"
#include "render/mesh/Mesh.h"
#include "render/shader/ShaderCache.h"
#include "render/debug/DebugDraw.h"
#include "render/debug/DebugUI.h"
#include "EnvironmentSettings.h"
#include "SceneBuffers.h"
#include "objects/Camera.h"

#include "resources/TextureResource.h"
#include "render/debug/DebugSettings.h"
#include "RenderModeUtils.h"
#include "utils/MeshGeneration.h"
#include "loader/FileLoader.h"

#include <functional>
#include "system/System.h"

#define USE_RPSL_JIT 1
#include "afx_common_helpers.hpp"

using namespace Device;
using namespace Resources;

namespace render {

	using namespace ECS;

	RendererResources::RendererResources()
	{
		brdf_lut = TextureResource::Linear(L"assets/Textures/LUT/ibl_brdf_lut.dds");

		uint32_t colors[16];
		for (auto& color : colors)
			color = 0xFFFF00FF;
		blank_texture = Device::Texture::Create(Device::TextureInitializer(4, 4, 4, colors, false));

		for (auto& color : colors)
			color = 0x000000;

		TextureInitializer cube_initializer(4, 4, 1, 6, Device::Format::R8G8B8A8_unorm, 1);
		cube_initializer.SetData(colors, sizeof(colors))
			.SetArray(false)
			.SetCube(true)
			.SetNumDimensions(2);

		for (int i = 0; i < 6; i++)
		{
			cube_initializer.AddCopy(
				vk::BufferImageCopy(0, 0, 0,
vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, i, 1),
vk::Offset3D(0, 0, 0),
vk::Extent3D(4, 4, 1)
));
		}

		blank_cube_texture = Device::Texture::Create(cube_initializer);

		full_screen_quad_mesh = std::make_unique<Mesh>(false, 3, true, "full screen quad");
		MeshGeneration::generateFullScreenQuad(full_screen_quad_mesh.get());
		full_screen_quad_mesh->createBuffer();

		particle_quad_mesh = Common::Handle<Mesh>(std::make_unique<Mesh>());
		MeshGeneration::generateParticleQuad(particle_quad_mesh.get());
		particle_quad_mesh->createBuffer();
	}


	RpsGraphHandle& RpsGraphHandle::operator=(RpsGraphHandle&& other)
	{
		if (this != &other)
		{
			rpsRenderGraphDestroy(handle);
			handle = other.handle;
			other.handle = RPS_NULL_HANDLE;
		}

		return *this;
	}

	RpsGraphHandle::RpsGraphHandle(RpsGraphHandle&& other)
	{
		*this = std::move(other);
	}

	RpsGraphHandle::RpsGraphHandle(const RpsRenderGraphCreateInfo& info)
	{
		rpsRenderGraphCreate(Engine::GetVulkanContext()->GetRpsDevice(), &info, &handle);
	}

	RpsGraphHandle::~RpsGraphHandle()
	{
		rpsRenderGraphDestroy(handle);
	}

	RpsSubprogram RpsGraphHandle::GetMainEntry() const
	{
		return rpsRenderGraphGetMainEntry(handle);
	}

	RpsSubprogramHandle& RpsSubprogramHandle::operator=(RpsSubprogramHandle&& other)
	{
		if (this != &other)
		{
			rpsProgramDestroy(handle);
			handle = other.handle;
			other.handle = RPS_NULL_HANDLE;
		}

		return *this;
	}

	RpsSubprogramHandle::RpsSubprogramHandle(RpsSubprogramHandle&& other)
	{
		*this = std::move(other);
	}

	RpsSubprogramHandle::RpsSubprogramHandle(const RpsProgramCreateInfo& info)
	{
		rpsProgramCreate(Engine::GetVulkanContext()->GetRpsDevice(), &info, &handle);
	}

	RpsSubprogramHandle::~RpsSubprogramHandle()
	{
		rpsProgramDestroy(handle);
	}

	void SceneRenderer::SetRadianceCubemap(Resources::Handle<Resources::TextureResource> cubemap)
	{
		renderer_resources->radiance_cubemap = cubemap;
	}

	void SceneRenderer::SetIrradianceCubemap(Resources::Handle<Resources::TextureResource> cubemap)
	{
		renderer_resources->irradiance_cubemap = cubemap;
	}

	SceneRenderer::~SceneRenderer() = default;

	SceneRenderer::SceneRenderer(Scene& scene, ShaderCache* shader_cache, DebugSettings* settings)
		: scene(scene)
		, shader_cache(shader_cache)
		, debug_settings(settings)
	{
		scene_buffers = std::make_unique<SceneBuffers>();
		renderer_resources = std::unique_ptr<RendererResources>(new RendererResources());

		environment_settings = std::make_unique<EnvironmentSettings>();
		auto* context = Engine::GetVulkanContext();
		context->AddRecreateSwapchainCallback(std::bind(&SceneRenderer::OnRecreateSwapchain, this, std::placeholders::_1, std::placeholders::_2));
	}

	void SceneRenderer::OnRecreateSwapchain(int32_t width, int32_t height)
	{
	}


	static ShaderBufferStruct::Camera GetCameraData(ICameraParamsProvider* camera, uvec2 screen_size)
	{
		ShaderBufferStruct::Camera result;
		result.projectionMatrix = camera->cameraProjectionMatrix();
		result.projectionMatrix[1][1] *= -1;
		result.position = camera->cameraPosition();
		result.viewMatrix = camera->cameraViewMatrix();
		result.screenSize = screen_size;
		result.zMin = camera->cameraZMinMax().x;
		result.zMax = camera->cameraZMinMax().y;
		return result;
	}

	static ShaderBufferStruct::EnvironmentSettings GetEnvSettings(const EnvironmentSettings& settings)
	{
		ShaderBufferStruct::EnvironmentSettings result;

		result.direction_light_enabled = false;
		result.environment_brightness = settings.environment_brightness;
		result.exposure = settings.exposure;
		return result;
	}

	static uint64_t CalcGuaranteedCompletedFrameIndexForRps()
	{
		auto context = Engine::GetVulkanContext();
		// For VK we wait for swapchain before submitting, so max queued frame count is swapChainImages + 1.
		const uint32_t maxQueuedFrames = uint32_t(context->GetSwapchainImageCount() + 1);

		return (context->GetCurrentFrame() > maxQueuedFrames) ? context->GetCurrentFrame() - maxQueuedFrames
			: RPS_GPU_COMPLETED_FRAME_INDEX_NONE;
	}


	tl::expected<RpsRenderGraph, std::string> SceneRenderer::LoadGraph(const char* path, bool compile)
	{
		std::error_code err;
		auto currentWorkingDir = std::filesystem::current_path(err);

		std::filesystem::path byteCodePath = std::filesystem::path(currentWorkingDir).append(path);
		if (byteCodePath.extension() != ".llvm.bc")
			byteCodePath.replace_extension(".llvm.bc");

		if (compile)
		{
			std::filesystem::path srcPath = std::filesystem::path(currentWorkingDir).append(path);
			if (srcPath.extension() != ".rpsl")
				srcPath.replace_extension(".rpsl");

			constexpr char* COMPILER_PATH = "rps/rps-hlslc.exe";
			std::stringstream commandline;

			commandline << COMPILER_PATH << " " << srcPath << " -O3 -rps-target-dll -rps-bc -m " << srcPath.filename().replace_extension("") << " -od " << srcPath.parent_path();

			if (!::System::LaunchProcess(commandline.str().c_str()))
			{
				return tl::unexpected(std::string("Error compiling ") + srcPath.string());
			}
		}

        const char*      argv[] = {""};
        RpsAfxJITHelper  jit(_countof(argv), argv);

        int32_t jitStartupResult = jit.pfnRpsJITStartup(1, argv);
		if (jitStartupResult != 0)
		{
			return tl::unexpected("JIT startup failed");
		}
        //RpsJITModule hJITModule = jit.LoadBitcode("D:\\code\\AMD\\RenderPipelineShaders\\build\\tests\\console\\test_rpsl_jit.llvm.bc");
        RpsJITModule hJITModule = jit.LoadBitcode(byteCodePath.string().c_str());
		if (!hJITModule)
		{
			return tl::unexpected("Failed loading jit bitcode");
		}

        auto moduleName = jit.GetModuleName(hJITModule);
        auto entryNameTable = jit.GetEntryNameTable(hJITModule);

        char         nameBuf[256];
        RpsRpslEntry hRpslEntry = jit.GetEntryPoint(
            hJITModule, rpsMakeRpslEntryName(nameBuf, std::size(nameBuf), moduleName, entryNameTable[0]));

		if (hRpslEntry == nullptr)
		{
			return tl::unexpected("Failed obtaining RpsRpsEntry");
		}

        RpsRenderGraphCreateInfo renderGraphCreateInfo            = {};
        renderGraphCreateInfo.scheduleInfo.scheduleFlags          = RPS_SCHEDULE_DISABLE_DEAD_CODE_ELIMINATION_BIT;
        renderGraphCreateInfo.mainEntryCreateInfo.hRpslEntryPoint = hRpslEntry;

        RpsRenderGraph renderGraph = {};
		if (rpsRenderGraphCreate(Engine::GetVulkanContext()->GetRpsDevice(), &renderGraphCreateInfo, &renderGraph) != RPS_OK)
		{
			return tl::unexpected("Failed to create render graph");
		}

		return renderGraph;
	}


	void SceneRenderer::RenderSceneGraph(RpsRenderGraph graph, gsl::span<const RpsConstant> args, gsl::span<const RpsRuntimeResource const*> resources)
	{
		OPTICK_EVENT();

        if (graph != RPS_NULL_HANDLE)
        {
			assert(args.size() == resources.size());

			RpsRenderGraphUpdateInfo updateInfo = {};
			updateInfo.numArgs = std::min(args.size(), resources.size());
			updateInfo.ppArgs = args.data();
			updateInfo.ppArgResources = resources.data();
			updateInfo.frameIndex = Engine::GetVulkanContext()->GetCurrentFrame();
			updateInfo.gpuCompletedFrameIndex = CalcGuaranteedCompletedFrameIndexForRps();
			updateInfo.diagnosticFlags = RPS_DIAGNOSTIC_ENABLE_RUNTIME_DEBUG_NAMES;
			if (Engine::GetVulkanContext()->GetCurrentFrame() < Engine::GetVulkanContext()->GetSwapchainImageCount())
			{
				updateInfo.diagnosticFlags = RPS_DIAGNOSTIC_ENABLE_ALL;
			}

			rpsRenderGraphUpdate(graph, &updateInfo);

			RpsRenderGraphBatchLayout batchLayout = {};
			RpsResult                 result = rpsRenderGraphGetBatchLayout(graph, &batchLayout);
			assert(result == RPS_OK);

			auto context = Engine::GetVulkanContext();
			context->ReserveSemaphores(batchLayout.numFenceSignals);

			for (uint32_t iBatch = 0; iBatch < batchLayout.numCmdBatches; iBatch++)
			{
				auto& batch = batchLayout.pCmdBatches[iBatch];

				auto* state = Engine::GetVulkanContext()->GetRenderState();

				state->BeginRecording(Device::PipelineBindPoint::Graphics);
				RpsRenderGraphRecordCommandInfo recordInfo = {};

				auto commandBuffer = state->GetCurrentCommandBuffer();
				recordInfo.pUserContext = state;
				recordInfo.cmdBeginIndex = batch.cmdBegin;
				recordInfo.numCmds = batch.numCmds;
				recordInfo.hCmdBuffer = rpsVKCommandBufferToHandle(commandBuffer);

				result = rpsRenderGraphRecordCommands(graph, &recordInfo);
				assert(result == RPS_OK);

				state->EndRecording();

				context->AddFrameCommandBuffer(Device::FrameCommandBufferData(*state, batch, batchLayout.pWaitFenceIndices));
			}
        }
	}

	Device::Texture* SceneRenderer::GetBlankTexture() const
	{
		return renderer_resources->blank_texture.get();
	}

}