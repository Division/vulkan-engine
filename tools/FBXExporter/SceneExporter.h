#pragma once
#include <unordered_map>
#include <string>
#include <fbxsdk.h>
#include "FBXExporterSettings.h"
#include "exporters/MeshExporter.h"
#include <physx/PxPhysicsAPI.h>
#include "scene/Physics.h"

namespace Exporter
{

	struct MeshExportData
	{
		enum class Type
		{
			Mesh,
			Phys,
			PhysConvex
		};


		std::string name;
		std::vector<SourceMesh> meshes;
		std::vector<FbxSkeleton*> bones;
		std::vector<SubMesh> submeshes;
		Type type = Type::Mesh;
		FbxNode* root_node = nullptr;
	};

	// Per-file export settings
	struct FileMetadata
	{
		float scale = 1;
		bool loaded = false;
	};

	class SceneExporter
	{
	public:
		SceneExporter(FBXExporterSetting settings);
		~SceneExporter();

		bool Export();

	private:
		bool ExportFBXFile(const std::filesystem::path& path);
		FileMetadata GetMetadataForFile(const std::filesystem::path& path);
		bool ExportRootNode(FbxScene* scene, const std::filesystem::path& path);
		std::unordered_map<std::string, MeshExportData> GetMeshesToExport(FbxScene* scene);
		std::wstring GetMeshOutputPath(const std::string& mesh_name, const std::filesystem::path& fbx_path);
		std::filesystem::path GetOutputPath(const std::filesystem::path& src_path);

		Physics::Handle<physx::PxFoundation> foundation;
		Physics::Handle<physx::PxCooking> cooking;

		bool input_is_in_assets;
		FbxManager* manager;
		FBXExporterSetting settings;
	};
}