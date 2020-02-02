#pragma once 

#include "CommonIncludes.h"

struct GLFWwindow;

namespace core
{
	namespace Device
	{
		class VulkanRenderState;
		class ShaderCache;
	}
}

namespace core { namespace render { 

	namespace DebugUI 
	{
		void Initialize(GLFWwindow* window, core::Device::ShaderCache* shader_cache);
		void Deinitialize();
		void NewFrame();
		void Update(float dt);
		void Render(core::Device::VulkanRenderState& state);

		bool WantCaptureMouse();
		bool WantCaptureKeyboard();

		void SetMainWidgetVisible(bool visible);

	} 

} }