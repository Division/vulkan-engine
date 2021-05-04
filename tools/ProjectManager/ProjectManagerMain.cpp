#include "Engine.h"
#include "memory/Profiler.h"
#include "render/debug/DebugUI.h"
#include "UI/AssetTree.h"
#include "lib/imgui/imgui.h"
#include <filesystem>
#include "utils/Dialogs.h"
#include "AssetCache/AssetCache.h"

namespace fs = std::filesystem;
using namespace UI;

class ProjectManager : public IGame {
public:
	ProjectManager()
		: asset_tree(cache)
	{
		SetCurrenProjectPath("C:/Users/pmuji/Documents/sources/vulkan-engine/assets/assets");
	}

	~ProjectManager()
	{

	}

	void init() override
	{
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
		if (ImGui::BeginMenu("Edit"))
		{
			if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
			if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
			ImGui::Separator();
			if (ImGui::MenuItem("Cut", "CTRL+X")) {}
			if (ImGui::MenuItem("Copy", "CTRL+C")) {}
			if (ImGui::MenuItem("Paste", "CTRL+V")) {}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Assets"))
		{
			if (ImGui::MenuItem("Export pending assets", "CTRL+E")) 
				ExportPendingAssets();

			//if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	void update(float dt) override
	{
		ShowMenu();
			

		ImGuiIO& io = ImGui::GetIO();

		ImGui::ShowDemoWindow();

		ImGui::Begin("Asset Manager");
		asset_tree.Update(dt);
		ImGui::End();
	}

	void ExportPendingAssets()
	{
		cache.ExportPending();
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