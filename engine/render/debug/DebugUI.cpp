#include "CommonIncludes.h"

#include "DebugUI.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "render/texture/Texture.h"
#include "render/buffer/VulkanBuffer.h";

namespace core { namespace render { 
	
	namespace DebugUI
	{
		using namespace Device;

		namespace 
		{
			std::unique_ptr<Texture> font_texture;
			//std::unique_ptr<VulkanBuffer>
		}

		void Initialize(GLFWwindow* window)
		{
			ImGui::CreateContext();
			ImGui_ImplGlfw_InitForVulkan(window, true);

			ImGuiIO& io = ImGui::GetIO();
			io.Fonts->AddFontDefault();

			unsigned char* pixels;
			int width, height;
			io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

			TextureInitializer initializer(width, height, 4, pixels, false);
			font_texture = std::make_unique<Texture>(initializer);
		}

		void NewFrame()
		{
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
		}

		void Update(float dt)
		{
			static float f = 0.0f;
			static int counter = 0;
			static bool show_demo_window = true;

			ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

			ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
			ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state

			vec3 clear_color(0.2, 0.2, 0.2);
			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
			ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

			if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
				counter++;
			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::End();

			ImGui::Render();
		}

		void Render(VulkanRenderState& state)
		{
			auto* draw_data = ImGui::GetDrawData();
		}

		void Deinitialize()
		{
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();
			font_texture = nullptr;
		}

	}

} }
