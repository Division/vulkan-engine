#include "CommonIncludes.h"
#include "imgui/imgui.h"
#include "render/renderer/EnvironmentSettings.h"
#include "ecs/components/Light.h"

namespace render {

	namespace DebugUI
	{
		void DrawEnvironmentEditorWidget(EnvironmentSettings& settings)
		{
			ImGui::Begin("Environment Editor");
			ImGui::SliderFloat("Exposure", &settings.exposure, 0.0, 2.0);
			ImGui::SliderFloat("Environment brightness", &settings.environment_brightness, 0.0, 2.0);

			if (settings.directional_light)
			{
				ImGui::Checkbox("Direction light enabled", &settings.directional_light->enabled);
			}

			ImGui::Checkbox("Bloom enabled", &settings.bloom_enabled);
			if (settings.bloom_enabled)
			{
				ImGui::SliderFloat("Bloom strength", &settings.bloom_strength, 0.0, 1.0);
				ImGui::SliderFloat("Bloom threshold", &settings.bloom_threshold, 0.0, 1.0);
				ImGui::SliderFloat("Bloom scatter", &settings.bloom_scatter, 0.0, 1.0);
				ImGui::SliderFloat("Bloom clamp", &settings.bloom_clamp_max, 0.0, 30000.0);
			}

			ImGui::End();
		}

	}


}