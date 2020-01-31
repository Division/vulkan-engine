#include "CommonIncludes.h"

#include "DebugUI.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "render/texture/Texture.h"

namespace core { namespace render { 
	
	namespace DebugUI
	{
		using namespace Device;

		namespace 
		{
			std::unique_ptr<Texture> font_texture;
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
			
			ImGui::Render();
		}

		void Render()
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
