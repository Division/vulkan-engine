#pragma once

#include <filesystem>

namespace Exporter
{

	struct FBXExporterSetting
	{
		std::filesystem::path bin_path;
		std::filesystem::path bin_assets_path;
		std::filesystem::path assets_path;
		std::filesystem::path input_path;
	};

}