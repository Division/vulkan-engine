#include "Engine.h"
#include "memory/Profiler.h"
#include "render/debug/DebugUI.h"
#include "UI/AssetTree.h"
#include "lib/imgui/imgui.h"
#include <filesystem>
#include "utils/Dialogs.h"
#include "AssetCache/AssetCache.h"
#include "AssetCache/AssetTypes.h"

namespace fs = std::filesystem;
using namespace UI;

class ProjectManager : public IGame {
public:
	ProjectManager()
		: asset_tree(cache)
	{
		
	}

	~ProjectManager()
	{

	}

	void init() override
	{
		SetCurrenProjectPath("C:/Users/pmuji/Documents/sources/vulkan-engine/assets/assets");
		render::DebugUI::SetEnvironmentWidgetVisible(false);
		render::DebugUI::SetMainWidgetVisible(false);
		render::DebugUI::SetEngineStatsVisible(false);
	}

	void ShowMenu()
	{
		ImGui::BeginMainMenuBar();

		if (ImGui::BeginMenu("Project"))
		{
			if (ImGui::MenuItem("Open"))
			{
				auto path = utils::BrowseFolder("C:\\games");
				if (!path.empty())
					SetCurrenProjectPath(path);
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Assets"))
		{
			if (ImGui::MenuItem("Refresh tree", "CTRL+R"))
				action = Action::ReloadTree;
			if (ImGui::MenuItem("Export pending assets", "CTRL+E"))
				action = Action::ExportPending;

			//if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	void update(float dt) override
	{
		if (action == Action::ReloadTree)
			RefreshTree();
		else if (action == Action::ExportPending)
			ExportPendingAssets();

		action = Action::None;

		ShowMenu();

		ImGuiIO& io = ImGui::GetIO();

		ImGui::ShowDemoWindow();

		ImGui::Begin("Asset Manager");
		asset_tree.Update(dt);
		ImGui::End();

		DrawSelectionInfo();
	}

	const char* GetExportType(const wchar_t *str)
	{
		if (!str) return "none";
		static Asset::UnorderedMapWChar<std::string> map;
		auto it = map.find(str);
		if (it != map.end())
			return it->second.c_str();

		auto inserted = map.insert({ str, utils::WStringToString(str) });
		return inserted.first->second.c_str();
	}

	void DrawSelectionInfo()
	{
		ImGui::Begin("Current selection");
		
		auto& selection = asset_tree.GetSelectedAssets();
		if (!selection.empty())
		{
			if (selection.size() == 1)
			{
				auto asset = *selection.begin();
				ImGui::BeginGroup();
				ImGui::Text("Source asset: ");
				ImGui::SameLine();
				ImGui::Text(asset->GetStrFilename().c_str());

				ImGui::Text("Export type: ");
				ImGui::SameLine();
				ImGui::Text(GetExportType(asset->GetType()));

				ImGui::Text("Exported assets");
				ImGui::BeginChild(ImGui::GetID((void*)(intptr_t)0), ImVec2(ImGui::GetWindowWidth() * 0.9f, 200.0f), true);
				//if (scroll_to)
					//ImGui::SetScrollFromPosY(ImGui::GetCursorStartPos().y + scroll_to_px, i * 0.25f);

				auto* cache_entry = asset->GetCacheEntry();
				if (cache_entry)
				{
					for (auto& output : cache_entry->outputs)
					{
						ImGui::Text(output.second->filename_str.c_str());
					}
				}

				ImGui::EndChild();
				ImGui::EndGroup();
			}
		}

		ImGui::End();
	}

	void RefreshTree()
	{
		asset_tree.DeselectAll();
		cache.Reload();
	}

	void ExportPendingAssets()
	{
		asset_tree.DeselectAll();
		cache.ExportPending();
		cache.Reload();
	}

	void cleanup() override
	{

	}

	void SetCurrenProjectPath(fs::path path)
	{
		cache.SetProjectDirectory(path);
		std::cout << "Project path set to: " << path << std::endl;
	}

private:
	ECS::EntityManager* manager = nullptr;
	ECS::TransformGraph* graph = nullptr;
	
	enum class Action
	{
		None,
		ReloadTree,
		ExportPending
	};

	Action action = Action::None;
	Asset::Cache cache;
	AssetTree asset_tree;
	fs::path current_project_path;
};

int main(int argc, char* argv[]) {

	Memory::Profiler::MakeSnapshot();
	{
		Engine engine(std::make_unique<ProjectManager>());
		engine.MainLoop();
	}
	Memory::Profiler::ValidateSnapshot();

	return 0;
}