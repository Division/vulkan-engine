#include "Engine.h"
#include "render/device/Device.h"
#include "render/device/VulkanContext.h"
#include "render/device/VulkanUploader.h"
#include "system/Logging.h"
#include "scene/Scene.h"
#include "render/renderer/SceneRenderer.h"
#include "system/Input.h"

namespace core
{

	Engine* Engine::instance;

	Engine::Engine(IGame& game) : game(game)
	{
		instance = this;
		ENGLogSetOutputFile("log.txt");

		if (!glfwInit())
		{
			std::cout << "init failure\n";
		}

		if (glfwVulkanSupported())
		{
			std::cout << "vk +";
		}
		else {
			std::cout << "vk -";
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		window = glfwCreateWindow(800, 600, "Vulkan engine", NULL, NULL);

		glfwSetWindowUserPointer(window, this);

		// Set callback functions
		glfwSetFramebufferSizeCallback(window, SizeCallback);
		//glfwSetWindowFocusCallback(mWindow, focus);
		
		int32_t width, height;
		glfwGetFramebufferSize(window, &width, &height);

		device = std::make_unique<Device::Device>(window);
		device->GetContext()->initialize();
		device->GetContext()->RecreateSwapChain();
		glfwSetTime(0);

		scene_renderer = std::make_unique<render::SceneRenderer>();
		scene = std::make_unique<Scene>();

		input = std::make_unique<system::Input>(window);
		game.init();
	}

	Engine::~Engine()
	{
		game.cleanup();
		GetVulkanContext()->Cleanup();
		device = nullptr;

		if (window)
			glfwDestroyWindow(window);

		glfwTerminate();
	}

	void Engine::SizeCallback(GLFWwindow* window, int32_t width, int32_t height)
	{
		auto* engine = (Engine*)glfwGetWindowUserPointer(window);
		engine->WindowResize(width, height);
		OutputDebugStringA((std::string() + std::to_string(width) + ", " + std::to_string(height) + "\n").c_str());
	}

	void Engine::WindowResize(int32_t width, int32_t height)
	{
		device->GetContext()->WindowResized();
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
			auto* context = device->GetContext();

			current_time = glfwGetTime();
			float dt = (float)(current_time - last_time);

			glfwPollEvents();
			input->update();
			scene->update(dt);
			game.update(dt);

			context->WaitForRenderFence();
			scene_renderer->RenderScene(scene.get());
			context->Present();
			glfwSwapBuffers(window);

			last_time = current_time;
		}

		// Wait idle before shutdown
		vkDeviceWaitIdle(device->GetContext()->GetDevice());
	}

	Device::VulkanContext* Engine::GetVulkanContext()
	{
		return instance->GetDevice()->GetContext();
	}

	vk::Device Engine::GetVulkanDevice()
	{
		return instance->GetDevice()->GetContext()->GetDevice();
	}

}

