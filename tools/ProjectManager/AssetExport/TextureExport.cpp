#include <cstdlib>
#include <sstream>

#include "TextureExport.h"
#include "system/FileSystem.h"
#include "utils/StringUtils.h"

namespace Asset::Export::Texture
{
	Result TextureExport::Export()
	{
		Result result;
		
		auto filename = asset_relative_path.filename();
		filename.replace_extension(L".dds");
		
		result.success = true;
		auto& asset = result.exported_assets.emplace_back();
		std::wstring lowercase_filename = filename;
		utils::Lowercase(lowercase_filename);
		asset.path = FileSystem::FormatPath(asset_relative_path.parent_path() / lowercase_filename);

		std::wstringstream command_line;

		const auto full_src_path = FileSystem::FormatPath(asset_base_path / asset_relative_path);
		const auto output_directory = FileSystem::FormatPath(asset_cache_directory);

		command_line << L".\\texconv.exe -f BC7_UNORM -l -y -o \"" << output_directory << L"\" \"" << FileSystem::FormatPath(full_src_path) << L"\"";

		auto command_line_formatted = command_line.str(); 
		utils::ReplaceAll(command_line_formatted, L"/", L"\\");

		const auto command_result = std::system(utils::WStringToString(command_line_formatted).c_str());
		if (command_result != 0)
			result.success = false;

		return result;
	}
}