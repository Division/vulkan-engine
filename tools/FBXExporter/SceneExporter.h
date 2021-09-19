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
			Skeleton,
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

	struct ExportedSceneAssets
	{
		std::vector<std::wstring> filenames;
	};

	class SceneExporter
	{
	public:
		SceneExporter(FBXExporterSetting settings, physx::PxFoundation* foundation = nullptr);
		~SceneExporter();

		bool Export();
		bool ExportFBXFile(const std::filesystem::path& path, ExportedSceneAssets* exported_assets = nullptr);
		bool ExportFBXAnimationFile(const std::filesystem::path& path, bool is_additive, ExportedSceneAssets* exported_assets = nullptr);

	private:
		FileMetadata GetMetadataForFile(const std::filesystem::path& path);
		bool ExportRootNode(FbxScene* scene, const std::filesystem::path& path, ExportedSceneAssets* exported_assets);
		bool ExportAnimation(const std::filesystem::path& path, ExportedSceneAssets* exported_assets, bool is_additive);
		std::unordered_map<std::string, MeshExportData> GetMeshesToExport(FbxScene* scene);
		std::wstring GetMeshOutputPath(const std::string& mesh_name, const std::filesystem::path& fbx_path);
		std::wstring GetSkeletonOutputPath(const std::filesystem::path& fbx_path);
		std::filesystem::path GetAnimationOutputPath(const std::filesystem::path& fbx_path);
		std::filesystem::path GetOutputPath(const std::filesystem::path& src_path);
		bool WriteSkeletonToFile(const std::filesystem::path& path, ozz::animation::Skeleton& skeleton);

		Physics::Handle<physx::PxFoundation> foundation;
		Physics::Handle<physx::PxCooking> cooking;

		bool input_is_in_assets;
		FbxManager* manager;
		FBXExporterSetting settings;
	};
}