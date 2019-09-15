#pragma once

#include "CommonIncludes.h"
#include "IGame.h"

namespace core 
{
	namespace Device 
	{
		class Device;
		class VulkanContext;
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

		void MainLoop();

	private:
		static Engine* instance;
		IGame& game;
		GLFWwindow* window;
		std::unique_ptr<Device::Device> device;

		bool loop_started = false;
		double last_time = 0;
		double current_time = 0;
	};
}
