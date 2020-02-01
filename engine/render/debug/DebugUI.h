#pragma once 

struct GLFWwindow;

namespace core
{
	namespace Device
	{
		class VulkanRenderState;
	}
}

namespace core { namespace render { 

	namespace DebugUI 
	{
		void Initialize(GLFWwindow* window);
		void Deinitialize();
		void NewFrame();
		void Update(float dt);
		void Render(Device::VulkanRenderState& state);

	} 

} }