#include "Engine.h"
#include "memory/Profiler.h"
#include "render/debug/DebugUI.h"
#include "UI/AssetTree.h"
#include "lib/imgui/imgui.h"
#include <filesystem>
#include "utils/Dialogs.h"
#include "AssetCache/AssetCache.h"
#include "AssetCache/AssetTypes.h"
#include "utils/SystemPathUtils.h"
#include "loader/FileLoader.h"
#include "utils/Dialogs.h"
#include "rapidjson/prettywriter.h"

namespace fs = std::filesystem;
using namespace UI;

const char* GetCachedCString(const wchar_t* str)
{
	if (!str) return "none";
	static Asset::UnorderedMapWChar<std::string> map;
	auto it = map.find(str);
	if (it != map.end())
		return it->second.c_str();

	auto inserted = map.insert({ str, utils::WStringToString(str) });
	return inserted.first->second.c_str();
}

class ProjectManager : public IGame {
public:
	ProjectManager()
		: asset_tree(cache)
	{

	}

	~ProjectManager()
	{

	}

	void LoadConfig()
	{
		auto storage_dir = utils::GetDocumentsFolder() / L"vkengine";
		if (!FileSystem::CreateDirectories(storage_dir))
			storage_dir = "./";

		config_path = storage_dir / "asset_manager.json";

		auto config_data = loader::LoadFile(config_path);
		if (config_data.size())
		{
			std::wstringstream sstream;
			sstream.write((wchar_t*)config_data.data(), config_data.size() / sizeof(wchar_t));

			Asset::Types::WDocument json;
			json.Parse(sstream.str().c_str());
			if (json.HasParseError() || !json.IsObject())
			{
				utils::ShowMessageBox("Error", "Failed loading asset_manager.json");
			}
			else
			{
				auto object = json.GetObject();

				if (object.HasMember(L"recent") && object[L"recent"].IsArray())
				{
					auto arr = object[L"recent"].GetArray();

					for (auto iter = arr.Begin(); iter != arr.End(); iter++)
					{
						if (!iter->IsString())
							continue;
						
						recent_projects.push_back({ iter->GetString(), utils::WStringToString(iter->GetString()) });
					}
				}
			}
		}
	}

	void SaveConfig()
	{
		Asset::Types::WDocument document;
		auto& allocator = document.GetAllocator();
		document.SetObject();
		auto& object = document.GetObject();

		auto array = Asset::Types::WValue(rapidjson::kArrayType);
		for (auto& path : recent_projects)
		{
			array.PushBack( Asset::Types::WValue().SetString(FileSystem::FormatPath(path.first).c_str(), allocator), allocator);
		}

		object.AddMember(L"recent", array, allocator);

		rapidjson::GenericStringBuffer<Asset::Types::WDocument::ValueType::EncodingType> buffer;
		rapidjson::PrettyWriter<decltype(buffer), Asset::Types::WDocument::ValueType::EncodingType, Asset::Types::WDocument::ValueType::EncodingType> writer(buffer);
		document.Accept(writer);

		std::wstring str(buffer.GetString());

		try
		{
			FileSystem::CreateDirectories(config_path.parent_path());
			std::ofstream output_stream(config_path, std::ofstream::binary);
			output_stream.exceptions(std::ofstream::badbit | std::ofstream::failbit);
			output_stream.write((char*)buffer.GetString(), buffer.GetSize());
		}
		catch (std::ofstream::failure e)
		{
			utils::ShowMessageBox("Error", ("Can't save config at path " + utils::WStringToString(config_path)).c_str());
		}
	}

	void init() override
	{
		LoadConfig();
		if (recent_projects.size())
			SetCurrenProjectPath(recent_projects[0].first);

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
				auto current_path = cache.GetProjectDirectory();
				utils::ReplaceAll(current_path, L"/", L"\\");
				auto path = utils::BrowseFolder(utils::WStringToString(current_path));
				if (!path.empty())
					SetCurrenProjectPath(path);
			}

			if (ImGui::BeginMenu("Recent"))
			{
				for (auto& dir : recent_projects)
				{
					if (ImGui::MenuItem(dir.second.c_str()))
					{
						SetCurrenProjectPath(dir.first);
					}
				}
				ImGui::EndMenu();
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
				ImGui::Text(GetCachedCString(asset->GetType()));

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
		auto it = std::find_if(recent_projects.begin(), recent_projects.end(), [&path](auto& a) { return a.first == path; });
		if (it != recent_projects.end())
			recent_projects.erase(it);

		recent_projects.push_front({ path, utils::WStringToString(path.wstring()) });

		cache.SetProjectDirectory(path);
		
		SaveConfig();
	}

private:
	ECS::EntityManager* manager = nullptr;
	ECS::TransformGraph* graph = nullptr;
	std::deque<std::pair<std::wstring, std::string>> recent_projects;
	
	enum class Action
	{
		None,
		ReloadTree,
		ExportPending
	};

	Action action = Action::None;
	Asset::Cache cache;
	AssetTree asset_tree;
	fs::path config_path;
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