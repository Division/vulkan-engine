#include <cstdlib>
#include <sstream>

#include "FBXExport.h"
#include "system/FileSystem.h"
#include "utils/StringUtils.h"
#include "SceneExporter.h"
#include "Engine.h"
#include "scene/Scene.h"
#include "scene/Physics.h"

namespace Asset::Export::FBX
{
	FBXExport::FBXExport(ExportType export_type, AnimationType animation_type)
		: export_type(export_type)
		, animation_type(animation_type)
	{}

	Result FBXExport::Export()
	{
		if (!FileSystem::CreateDirectories(asset_cache_directory))
		{
			return {};
		}

		Result result;
		Exporter::FBXExporterSetting settings;

		settings.input_path = FileSystem::FormatPath(asset_base_path / asset_relative_path);
		settings.assets_path = asset_base_path;
		settings.bin_assets_path = cache_path;

		Exporter::SceneExporter scene_exporter(settings, Engine::Get()->GetScene()->GetPhysics()->GetFoundation());

		Exporter::ExportedSceneAssets exported_assets;
		if (export_type == ExportType::Mesh)
			result.success = scene_exporter.ExportFBXFile(settings.input_path, &exported_assets);
		else
			result.success = scene_exporter.ExportFBXAnimationFile(settings.input_path, animation_type == AnimationType::Additive, &exported_assets);

		if (result.success)
		{
			for (auto& exported_filename : exported_assets.filenames)
			{
				auto& asset = result.exported_assets.emplace_back();
				asset.path = FileSystem::GetRelativePath(exported_filename, cache_path);
			}
		}

		return result;
	}

}