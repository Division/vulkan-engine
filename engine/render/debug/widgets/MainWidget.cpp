#include "CommonIncludes.h"
#include "imgui/imgui.h"
#include "render/debug/DebugUI.h"
#include "render/debug/DebugSettings.h"
#include "Engine.h"
#include "scene/Scene.h"
#include "objects/Camera.h"
#include "scene/Physics.h"

namespace render {

	namespace DebugUI
	{
		void DrawMainWidget(DebugSettings* debug_settings)
		{
			static bool show_demo_window = false;

			if (show_demo_window)
				ImGui::ShowDemoWindow(&show_demo_window);

			ImGui::Begin("Debug UI");
			ImGui::Checkbox("Demo Window", &show_demo_window);
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

			if (ImGui::Button("Next profiler"))
			{
				SwitchEngineStats();
			}

			if (debug_settings)
			{
				if (ImGui::Checkbox("Show Clusters", &debug_settings->draw_clusters))
				{
					if (debug_settings->draw_clusters)
					{
						debug_settings->cluster_matrix = glm::inverse(Engine::Get()->GetScene()->GetCamera()->viewMatrix());
					}
				}

				ImGui::Checkbox("Show OBB", &debug_settings->draw_bounding_boxes);
				ImGui::Checkbox("Show skeletons", &debug_settings->draw_skeletons);
				ImGui::Checkbox("Show Lights", &debug_settings->draw_lights);
				
				auto* physics = Engine::Get()->GetScene()->GetPhysics();
				bool physics_debug = physics->GetDebugRenderEnabled();
				if (ImGui::Checkbox("Physics", &physics_debug))
				{
					physics->SetDebugRenderEnabled(physics_debug);
				}
			}

			ImGui::End();
		}

	}


}