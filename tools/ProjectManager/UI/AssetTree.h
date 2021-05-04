#pragma once

#include "system/FileSystem.h"


namespace Asset
{
	class Cache;
}

namespace UI
{
	class AssetTree
	{
	public:
		AssetTree(Asset::Cache& cache);

		void Update(float dt);

	private:
		void DrawDirectory(const FileSystem::FileTree::DirectoryNode* node);

	private:

		Asset::Cache& cache;
	};
}