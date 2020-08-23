#pragma once
#include <unordered_map>
#include <string>
#include <fbxsdk.h>
#include "FBXExporterSettings.h"
#include "exporters/MeshExporter.h"

namespace Exporter
{

	struct MeshExportData
	{
		std::string name;
		std::vector<FbxMesh*> meshes;
		std::vector<FbxSkeleton*> bones;
		std::vector<SubMesh> submeshes;
	};

	class SceneExporter
	{
	public:
		SceneExporter(FBXExporterSetting settings);
		~SceneExporter();

		bool Export();

	private:
		bool ExportFBXFile(const std::filesystem::path& path);
		bool ExportRootNode(FbxScene* scene, const std::filesystem::path& path);
		std::unordered_map<std::string, MeshExportData> GetMeshesToExport(FbxScene* scene);
		std::wstring GetMeshOutputPath(const std::string& mesh_name, const std::filesystem::path& fbx_path);
		std::filesystem::path GetOutputPath(const std::filesystem::path& src_path);

		bool input_is_in_assets;
		FbxManager* manager;
		FBXExporterSetting settings;
	};
}