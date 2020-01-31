#pragma once 

struct GLFWwindow;

namespace core { namespace render { 

	namespace DebugUI 
	{

		void Initialize(GLFWwindow* window);
		void Deinitialize();
		void NewFrame();
		void Update(float dt);
		void Render();

	} 

} }