#include "Engine.h"
#include "render/device/Device.h"
#include "render/device/VulkanContext.h"
#include "render/device/VulkanUploader.h"
#include "system/Logging.h"
#include "scene/Scene.h"
#include "render/renderer/SceneRenderer.h"

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

		/*
		setInputCallbacksGLFW(mWindow);
		glfwSetWindowUserPointer(mWindow, this);

		// Set callback functions
		glfwSetFramebufferSizeCallback(mWindow, reshape);
		glfwSetWindowFocusCallback(mWindow, focus);
		*/

		int32_t width = 0;
		int32_t height = 0;
		glfwGetFramebufferSize(window, &width, &height);
		//reshape(window, width, height);

		device = std::make_unique<Device::Device>(window);
		device->GetContext()->initialize();
		glfwSetTime(0);

		scene_renderer = std::make_unique<render::SceneRenderer>();
		scene = std::make_unique<Scene>();

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
			game.update(dt);
			scene_renderer->RenderScene(scene.get());
			context->Present();

			glfwSwapBuffers(window);
			last_time = current_time;
		}

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

