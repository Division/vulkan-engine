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

		if (!entry.GetCacheEntry())
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

		for (auto& item : metadata->entries)
		{
			auto& entry = *item.second;
			auto& filename = item.second->GetStrFilename();

			if (entry.GetHidden())
				continue;

			render::DebugUI::ScopedTextColor color(GetColor(entry));
			auto node_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
			ImGui::TreeNodeEx(filename.c_str(), node_flags);
		}
	}

	void AssetTree::Update(float dt)
	{
		if (ImGui::TreeNode("Assets"))
		{
			auto file_tree_node = cache.GetFileTree()->GetRootNode();

			DrawDirectory(file_tree_node);

			ImGui::TreePop();
		}
	}
}