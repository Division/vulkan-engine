#include "imgui/imgui.h"
#include "memory/Profiler.h"
#include <string>
#include <sstream>

namespace core { namespace render { namespace DebugUI {

    std::array<std::pair<size_t, std::string>, 4> size_const = { std::make_pair(1, "b"), std::make_pair(1024, "kb"), std::make_pair(1024 * 1024, "mb"), std::make_pair(1024 * 1024 * 1024, "gb") };

    namespace 
    {
        auto& GetSizePair(size_t size)
        {
            for (int i = 1; i < size_const.size(); i++)
                if (size_const[i].first > size)
                    return size_const[i - 1];
            return size_const.back();
        }

        void AppendSize(size_t size, std::stringstream& stream)
        {
            auto& size_pair = GetSizePair(size);
            float float_size = (float)size / size_pair.first;
            stream.precision(2);
            stream.setf(std::ios::fixed);
            stream << float_size << " " << size_pair.second;
        }

        void AppendAllocations(size_t allocations, std::stringstream& stream)
        {
            stream << " (" << allocations << ")";
        }
    }


	void EngineStatsMemory()
	{
        const float DISTANCE = 10.0f;
        static int corner = 0;
        ImGuiIO& io = ImGui::GetIO();
        if (corner != -1)
        {
            ImVec2 window_pos = ImVec2((corner & 1) ? io.DisplaySize.x - DISTANCE : DISTANCE, (corner & 2) ? io.DisplaySize.y - DISTANCE : DISTANCE);
            ImVec2 window_pos_pivot = ImVec2((corner & 1) ? 1.0f : 0.0f, (corner & 2) ? 1.0f : 0.0f);
            ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
        }
        ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
        bool open = true;
        if (ImGui::Begin("Memory info", &open, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoInputs))
        {
            ImGui::Text("Memory");
            ImGui::Separator();

            std::stringstream stream;

            for (int i = 0; i < Memory::Tag::Count; i++)
            {
                stream.clear();
                stream.str("");
                stream << Memory::Profiler::GetTagName(Memory::Tag(i)) << ": ";
                AppendSize(Memory::Profiler::GetAllocationsData(Memory::Tag(i)).allocated_size, stream);
                AppendAllocations(Memory::Profiler::GetAllocationsData(Memory::Tag(i)).num_allocations, stream);
                    
                ImGui::Text(stream.str().c_str());
            }

            ImGui::Separator();
            stream.clear();
            stream.str("");
            stream << "Frame allocations: ";
            AppendSize(Memory::Profiler::GetFrameAllocationsSize(), stream);
            AppendAllocations(Memory::Profiler::GetFrameAllocations(), stream);

            ImGui::Text(stream.str().c_str());
        }
        ImGui::End();
	}

} } }