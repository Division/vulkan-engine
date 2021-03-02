#include "CommonIncludes.h"
#include "imgui/imgui.h"
#include "render/renderer/EnvironmentSettings.h"

namespace render {

	namespace DebugUI
	{
		void DrawEnvironmentEditorWidget(EnvironmentSettings& settings)
		{
			ImGui::Begin("Environment Editor");
			ImGui::SliderFloat("Exposure", &settings.exposure, 0.0, 2.0);
			ImGui::SliderFloat("Environment brightness", &settings.environment_brightness, 0.0, 2.0);
			ImGui::End();
		}

	}


}