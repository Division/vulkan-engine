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

			bool direction_light_enabled = settings.direction_light_color.w > 0.5f;
			ImGui::Checkbox("Direction light enabled", &direction_light_enabled);
			settings.direction_light_color.w = direction_light_enabled ? 1.0f : 0.0f;
			ImGui::End();
		}

	}


}