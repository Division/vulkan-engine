#pragma once

#include <string>

namespace Asset::Types
{
	class AssetEntry;
}

namespace Asset::Export
{
	class ExportCache
	{
	public:
		ExportCache(const std::wstring& path);

		void AddAsset(Types::AssetEntry& entry);

	private:
		std::wstring path;
	};
}