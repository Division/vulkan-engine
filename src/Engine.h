#pragma once

#include "CommonIncludes.h"
#include "IGame.h"

class Scene;

namespace core 
{
	namespace Device 
	{
		class Device;
		class VulkanContext;
	}

	namespace render
	{
		class SceneRenderer;
	}

	namespace system
	{
		class Input;
	}


	class Engine : public NonCopyable
	{
	public:
		static Engine* Get() { return instance; };
		static Device::VulkanContext* GetVulkanContext();
		static vk::Device GetVulkanDevice();

		Engine(IGame& game);
		~Engine();
		
		double time() const { return current_time; }

		Device::Device* GetDevice() { return device.get(); }
		Scene* GetScene() const { return scene.get(); }
		system::Input* GetInput() const { return input.get(); }

		void MainLoop();

	private:
		static Engine* instance;
		IGame& game;
		GLFWwindow* window;
		std::unique_ptr<Device::Device> device;
		std::unique_ptr<Scene> scene;
		std::unique_ptr<render::SceneRenderer> scene_renderer;
		std::unique_ptr<system::Input> input;

		bool loop_started = false;
		double last_time = 0;
		double current_time = 0;
	};
}
