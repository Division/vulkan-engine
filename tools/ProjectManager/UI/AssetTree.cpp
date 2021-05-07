#include "AssetTree.h"
#include "AssetCache/AssetCache.h"
#include "lib/imgui/imgui.h"
#include "render/debug/ImGUIHelper.h"
#include "AssetCache/AssetTypes.h"

using namespace Asset;

namespace UI
{
	AssetTree::AssetTree(Asset::Cache& cache)
		:cache(cache)
	{

	}

	ImU32 GetColor(Types::AssetEntry& entry)
	{
		if (!entry.ShouldSerialize())
			return 0xFF555555;

		if (entry.IsMissing())
			return 0xFF4444BB;

		if (entry.GetNeedsReexport())
			return 0xFF33FFFF;

		return ~0;
	}

	void AssetTree::DrawDirectory(const FileSystem::FileTree::DirectoryNode* node)
	{
		for (auto& item : node->directories)
		{
			if (ImGui::TreeNode(item.path_char.c_str()))
			{
				DrawDirectory(&item);
				ImGui::TreePop();
			}
		}

		auto* metadata = static_cast<FolderMetadata*>(node->user_data);

		if (!metadata)
			return;

		for (auto& item : metadata->entries)
		{
			auto& entry = *item.second;

			auto& filename = item.second->GetStrFilename();

			if (entry.GetHidden())
				continue;

			render::DebugUI::ScopedTextColor color(GetColor(entry));
			auto node_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
			if (selected_entries.find(&entry) != selected_entries.end())
				node_flags |= ImGuiTreeNodeFlags_Selected;

			ImGui::TreeNodeEx(filename.c_str(), node_flags);
			if ((ImGui::IsItemClicked()))
				clicked_entry = &entry;
		}
	}

	void AssetTree::Update(float dt)
	{
		if (ImGui::TreeNode("Assets"))
		{
			auto file_tree_node = cache.GetFileTree()->GetRootNode();

			clicked_entry = nullptr;
			DrawDirectory(file_tree_node);
			ProcessSelection();

			ImGui::TreePop();
		}
	}

	void AssetTree::ProcessSelection()
	{
		if (!clicked_entry)
			return;

		auto it = selected_entries.find(clicked_entry);
		const bool not_in_selection = it == selected_entries.end();

		if (ImGui::GetIO().KeyCtrl)
		{
			if (not_in_selection)
				selected_entries.insert(clicked_entry);
			else
				selected_entries.erase(clicked_entry);
		}
		else
		{
			selected_entries.clear();
			selected_entries.insert(clicked_entry);
		}
	}

	void AssetTree::DeselectAll()
	{
		selected_entries.clear();
	}
}