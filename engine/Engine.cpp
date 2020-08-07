#include "Engine.h"
#include "render/device/VulkanContext.h"
#include "render/device/VulkanUploader.h"
#include "system/Logging.h"
#include "scene/Scene.h"
#include "render/renderer/SceneRenderer.h"
#include "system/Input.h"
#include "render/shader/ShaderCache.h"
#include "render/material/MaterialManager.h"
#include "render/debug/DebugDraw.h"
#include "render/debug/DebugUI.h"
#include "memory/Profiler.h"
#include "memory/Containers.h"
#include "resources/ResourceCache.h"
#include "render/device/Resource.h"
#include "Handle.h"

Engine* Engine::instance;

struct temp
{
	std::vector<int> v;
	int i;
	std::string s;

	temp(int i, std::string s) : i(i), s(s) {}
};

Engine::Engine(std::unique_ptr<IGame> game) : game(std::move(game))
{
	instance = this;
	ENGLogSetOutputFile("log.txt");

	if (!glfwInit())
	{
		std::cout << "init failure\n";
	}

	if (glfwVulkanSupported())
	{
		std::cout << "Vulkan supported";
	}
	else {
		std::cout << "Vulkan unsupported";
	}

	auto working_dir = std::filesystem::current_path();
	std::cout << "working directory: " << working_dir << std::endl;

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(800, 600, "Vulkan engine", NULL, NULL);

	glfwSetWindowUserPointer(window, this);

	// Set callback functions
	glfwSetFramebufferSizeCallback(window, SizeCallback);
	//glfwSetWindowFocusCallback(mWindow, focus);
		
	int32_t width, height;
	glfwGetFramebufferSize(window, &width, &height);

	vulkan_context = std::make_unique<Device::VulkanContext>(window);
	vulkan_context->initialize();
	glfwSetTime(0);

	scene = std::make_unique<Scene>();
	shader_cache = std::make_unique<Device::ShaderCache>();
	debug_draw = std::make_unique<render::DebugDraw>(*shader_cache);
	scene_renderer = std::make_unique<render::SceneRenderer>(*scene, shader_cache.get());
	material_manager = std::make_unique<render::MaterialManager>();
	input = std::make_unique<System::Input>(window);

	vulkan_context->RecreateSwapChain(); // creating swapchain after scene renderer to handle subscribtion to the recreate event

	render::DebugUI::Initialize(window, shader_cache.get(), *scene_renderer->GetEnvironmentSettings());

	this->game->init();
}

Engine::~Engine()
{
	render::DebugUI::Deinitialize();

	game->cleanup();
	game = nullptr;

	input = nullptr;
	scene = nullptr;
	material_manager = nullptr;
	scene_renderer = nullptr;
	shader_cache = nullptr;
	debug_draw = nullptr;

	Resources::Cache::Get().Destroy();
	Common::GetReleaser().Clear();
	::Device::GetReleaser().Clear();
	vulkan_context->Cleanup();
	vulkan_context = nullptr;

	if (window)
		glfwDestroyWindow(window);

	glfwTerminate();

	OPTICK_SHUTDOWN();
}

void Engine::SizeCallback(GLFWwindow* window, int32_t width, int32_t height)
{
	auto* engine = (Engine*)glfwGetWindowUserPointer(window);
	engine->WindowResize(width, height);
}

void Engine::WindowResize(int32_t width, int32_t height)
{
	GetContext()->WindowResized();
}

void Engine::MainLoop()
{
	if (loop_started)
		throw new std::runtime_error("Main loop already started");

	loop_started = true;

	if (!window)
	{
		return;
	}

	while (!glfwWindowShouldClose(window))
	{
		Memory::Profiler::StartFrame();

		OPTICK_FRAME("MainThread");

		auto* context = GetContext();

		current_time = glfwGetTime();
		float dt = (float)(current_time - last_time);

		glfwPollEvents();
			
		render::DebugUI::NewFrame();

		input->update();
		scene->update(dt);
		game->update(dt);
		debug_draw->Update();

		render::DebugUI::Update(dt);

		context->WaitForRenderFence();
		scene_renderer->RenderScene();
		context->Present();
		glfwSwapBuffers(window);

		::Device::GetReleaser().Swap();
		Common::GetReleaser().Swap();

		last_time = current_time;
	}

	// Wait idle before shutdown
	vkDeviceWaitIdle(GetContext()->GetDevice());
}

Device::VulkanContext* Engine::GetVulkanContext()
{
	return instance->GetContext();
}

vk::Device Engine::GetVulkanDevice()
{
	return instance->GetContext()->GetDevice();
}

ECS::EntityManager* Engine::GetEntityManager() const
{
	return scene->GetEntityManager();
}
	
ECS::TransformGraph* Engine::GetTransformGraph() const
{
	return scene->GetTransformGraph();
}


