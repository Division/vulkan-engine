#pragma once

#include "system/FileSystem.h"
#include <unordered_set>

namespace Asset
{
	class Cache;
	
	namespace Types
	{
		class AssetEntry;
	}
}

namespace UI
{
	class AssetTree
	{
	public:
		AssetTree(Asset::Cache& cache);

		void Update(float dt);

		const std::unordered_set<Asset::Types::AssetEntry*> GetSelectedAssets() const { return selected_entries; }

		void DeselectAll();

	private:
		void DrawDirectory(const FileSystem::FileTree::DirectoryNode* node);
		void ProcessSelection();

	private:
		Asset::Types::AssetEntry* clicked_entry = nullptr;
		std::unordered_set<Asset::Types::AssetEntry*> selected_entries;
		Asset::Cache& cache;
	};
}