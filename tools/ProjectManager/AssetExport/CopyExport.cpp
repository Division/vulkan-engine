#include <cstdlib>
#include <sstream>

#include "CopyExport.h"
#include "system/FileSystem.h"
#include "utils/StringUtils.h"

namespace Asset::Export::Copy
{
	Result CopyExport::Export()
	{
		Result result;
		
		auto filename = asset_relative_path.filename();
		
		result.success = true;
		
		auto& asset = result.exported_assets.emplace_back();
		asset.path = asset_relative_path;

		const auto full_src_path = FileSystem::FormatPath(asset_base_path / asset_relative_path);
		const auto output_dir = FileSystem::FormatPath((cache_path / asset_relative_path));

		if (!FileSystem::CopyFile(full_src_path, output_dir))
			result.success = false;

		return result;
	}
}