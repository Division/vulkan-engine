#include "CommonIncludes.h"
#include "imgui/imgui.h"
#include "render/debug/DebugUI.h"

namespace render {

	namespace DebugUI
	{
		void DrawMainWidget()
		{
			static bool show_demo_window = false;

			if (show_demo_window)
				ImGui::ShowDemoWindow(&show_demo_window);

			ImGui::Begin("Debug UI");
			ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

			if (ImGui::Button("Next profiler"))
			{
				SwitchEngineStats();
			}

			ImGui::End();
		}

	}


}