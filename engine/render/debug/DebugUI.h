#pragma once 

#include "CommonIncludes.h"

struct GLFWwindow;

namespace Device
{
	class VulkanRenderState;
	class ShaderCache;
}

namespace render
{
	struct EnvironmentSettings;
}

namespace render { 

	namespace DebugUI 
	{
		void Initialize(GLFWwindow* window, Device::ShaderCache* shader_cache, EnvironmentSettings& environment_settings);
		void Deinitialize();
		void NewFrame();
		void Update(float dt);
		void Render(Device::VulkanRenderState& state);
		void SwitchEngineStats();

		bool WantCaptureMouse();
		bool WantCaptureKeyboard();

		void SetMainWidgetVisible(bool visible);
		void SetEnvironmentWidgetVisible(bool visible);

	} 

}