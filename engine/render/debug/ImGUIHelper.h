#pragma once

#include "imgui/imgui.h"

namespace render::DebugUI
{

	struct ScopedTextColor
	{
		ScopedTextColor(ImU32 color) { ImGui::PushStyleColor(ImGuiCol_Text, color); }
		~ScopedTextColor() { ImGui::PopStyleColor(); }
	};

}