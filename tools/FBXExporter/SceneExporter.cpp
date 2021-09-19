#include <iostream>
#include "SceneExporter.h"
#include "utils/StringUtils.h"
#include "SceneExporterUtils.h"
#include <fbxsdk/utils/fbxgeometryconverter.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/offline/fbx/fbx_skeleton.h>
#include <ozz/animation/offline/fbx/fbx_animation.h>
#include <ozz/animation/offline/fbx/fbx.h>
#include <ozz/animation/offline/animation_optimizer.h>
#include <ozz/animation/offline/additive_animation_builder.h>
#include <ozz/animation/offline/animation_builder.h>
#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/offline/skeleton_builder.h>
#include "ozz/base/io/archive.h"
#include "ozz/base/io/stream.h"
#include "scene/Physics.h"
#include "system/FileSystem.h"

namespace fs = std::filesystem;

using namespace physx;
using namespace Physics;
using namespace ozz::animation::offline::fbx;
using namespace ozz::animation::offline;
using namespace ozz::animation;

namespace Exporter
{
	namespace
	{
		const std::string mesh_prefix = "mesh_"; // goes to .mesh file
		const std::string phys_convex_prefix = "phys_convex_"; // Exported to .phys file
		const std::string phys_mesh_prefix = "phys_mesh_"; // Exported to .phys file
		const std::string default_key = "_default";
		const std::wstring mesh_extension = L".mesh";
		const std::wstring phys_extension = L".phys";
		const std::wstring skeleton_extension = L".skel";
		const std::wstring animation_extension = L".anim";

		auto export_prefixes = { mesh_prefix, phys_convex_prefix, phys_mesh_prefix };
	}

	class ErrorCallback : public PxErrorCallback
	{
	public:
		void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line) override
		{
			std::cerr << message << std::endl << "file: " << file << ", line: " << line << std::endl;
		}
	};

	SceneExporter::SceneExporter(FBXExporterSetting settings, physx::PxFoundation* in_foundation) : settings(settings)
	{
		manager = FbxManager::Create();
		FbxIOSettings *ios = FbxIOSettings::Create(manager, IOSROOT);
		manager->SetIOSettings(ios);
		input_is_in_assets = utils::BeginsWith(FileSystem::FormatPath(settings.input_path), FileSystem::FormatPath(settings.assets_path));

		static PxDefaultAllocator DefaultAllocator;
		static ErrorCallback ErrorCallback;

		if (!in_foundation)
		{
			foundation = Handle(PxCreateFoundation(PX_PHYSICS_VERSION, DefaultAllocator, ErrorCallback));
			if (!foundation)
				throw std::runtime_error("PxCreateFoundation failed!");

			in_foundation = foundation.get();
		}

		PxTolerancesScale scale;
		cooking = PxCreateCooking(PX_PHYSICS_VERSION, *in_foundation, PxCookingParams(scale));
		if (!cooking)
			throw std::runtime_error("PxCreateCooking failed!");
	}

	SceneExporter::~SceneExporter()
	{
		manager->Destroy();
	}

	bool SceneExporter::Export()
	{
		if (fs::is_directory(settings.input_path))
		{
			std::cout << "Directories not supported.\n";
			return false;
		}
		else
		{
			if (!fs::exists(settings.input_path))
			{
				std::cout << "Input file doesn't exist: " << settings.input_path << std::endl;
				return false;
			}
		}
		
		return ExportFBXFile(settings.input_path);
	}

	bool SceneExporter::ExportFBXFile(const std::filesystem::path& path, ExportedSceneAssets* exported_assets)
	{
		// Create an importer using the SDK manager.
		FbxImporter* importer = FbxImporter::Create(manager,"");

		// Use the first argument as the filename for the importer.
		if(!importer->Initialize(path.string().c_str(), -1, manager->GetIOSettings())) 
		{
			printf("Call to FbxImporter::Initialize() failed.\n"); 
			printf("Error returned: %s\n\n", importer->GetStatus().GetErrorString()); 
			return false;
		}

		// Create a new scene so that it can be populated by the imported file.
		FbxScene* scene = FbxScene::Create(manager, "scene");

		// Import the contents of the file into the scene.
		importer->Import(scene);
		importer->Destroy();

		// Get the current scene units (incoming from FBX file)
		FbxSystemUnit SceneSystemUnit = scene->GetGlobalSettings().GetSystemUnit();

		FbxGeometryConverter converter(manager);
		if (!converter.Triangulate(scene, true))
			std::wcout << L"[Warning] triangulation finished with errors.\n";

		if (!scene->GetRootNode())
		{
			std::wcout << L"Scene doesn't have root node" << std::endl;
			return false;
		}

		return ExportRootNode(scene, path, exported_assets);
	}

	bool SceneExporter::ExportFBXAnimationFile(const std::filesystem::path& path, bool is_additive, ExportedSceneAssets* exported_assets)
	{
		try
		{
			return ExportAnimation(path, exported_assets, is_additive);
		}
		catch (std::runtime_error e)
		{
			if (exported_assets)
				exported_assets->filenames.clear();

			std::cout << "Error exporting animation: " << e.what() << "\n";
			return false;
		}
	}

	FileMetadata SceneExporter::GetMetadataForFile(const std::filesystem::path& path)
	{
		FileMetadata result;

		fs::path json_path = path.wstring() + L".json";
		rapidjson::Document json;
		if (LoadJSONFile(json_path, json))
		{
			if (json.HasMember("scale"))
				result.scale = json["scale"].GetFloat();
		}

		return result;
	}

	std::unordered_map<std::string, MeshExportData> SceneExporter::GetMeshesToExport(FbxScene* scene)
	{
		std::unordered_map<std::string, MeshExportData> result;
		std::string current_name = default_key;
		bool nested_depth = 0;

		IterateNodesDepthFirst(scene->GetRootNode(), [&](FbxNode* node, uint32_t depth) {
			std::string name = node->GetName();
			if (name.size() > 0 && name[0] == '_') return false; // ignore underscored and it's children

			if (depth <= nested_depth)
				current_name = default_key;

			if (utils::BeginsWith(name, mesh_prefix))
			{
				if (current_name != default_key)
					throw std::runtime_error("Nested export groups not supported");

				current_name = name.substr(mesh_prefix.size());
				utils::Lowercase(current_name);

				if (result.find(current_name) != result.end())
					throw std::runtime_error("Export group already defined: " + current_name);

				result[current_name].root_node = node;
				result[current_name].type = MeshExportData::Type::Mesh;
				nested_depth = depth;
			}
			else
			if (utils::BeginsWith(name, phys_convex_prefix))
			{
				if (current_name != default_key)
					throw std::runtime_error("Nested export groups not supported");

				current_name = name.substr(phys_convex_prefix.size());
				utils::Lowercase(current_name);

				if (result.find(current_name) != result.end())
					throw std::runtime_error("Export group already defined: " + current_name);

				result[current_name].root_node = node;
				result[current_name].type = MeshExportData::Type::PhysConvex;
				nested_depth = depth;
			}

			for (int i = 0; i < node->GetNodeAttributeCount(); i++)
			{
				auto* attrib = node->GetNodeAttributeByIndex(i);
				if (attrib->GetAttributeType() == FbxNodeAttribute::eMesh)
				{
					attrib->SetName(node->GetName());
					result[current_name].meshes.push_back(SourceMesh(node, (FbxMesh*)attrib));
				}
				if (attrib->GetAttributeType() == FbxNodeAttribute::eSkeleton)
				{
					result[current_name].bones.push_back((FbxSkeleton*)attrib);
					result[current_name].type = MeshExportData::Type::Skeleton; // probably wrong
				}
			}

			return true;
		});

		return result;
	}

	bool ExtractOzzRuntimeSkeleton(FbxScene* scene, FbxNode* node, ozz::unique_ptr<Skeleton>& out_skeleton)
	{
		//ozz::animation::
		FbxSystemConverter converter(scene->GetGlobalSettings().GetAxisSystem(), scene->GetGlobalSettings().GetSystemUnit());
		RawSkeleton raw_skeleton;
		OzzImporter::NodeType node_type;
		node_type.skeleton = true;
		node_type.geometry = false;
		node_type.camera = false;
		ExtractSkeleton(node, &converter, node_type, &raw_skeleton);

		SkeletonBuilder builder;
		out_skeleton = builder(raw_skeleton);
		if (!out_skeleton) {
			return false;
		}

		return true;
	}

	bool SceneExporter::ExportAnimation(const std::filesystem::path& path, ExportedSceneAssets* exported_assets, bool is_additive)
	{
		FbxManagerInstance fbx_manager;
		FbxAnimationIOSettings settings(fbx_manager);
		::FbxSceneLoader* scene_loader;
		scene_loader = ozz::New<FbxSceneLoader>(path.string().c_str(), "", fbx_manager, settings);

		ozz::unique_ptr<Skeleton> skeleton;
		const bool has_skeleton = ExtractOzzRuntimeSkeleton(scene_loader->scene(), scene_loader->scene()->GetRootNode(), skeleton);
		if (!has_skeleton)
			return true;

		auto animation_names = ozz::animation::offline::fbx::GetAnimationNames(*scene_loader);
		if (animation_names.empty())
			return true;

		auto& animation_name = animation_names[0];

		RawAnimation animation;
		animation.name = animation_name;

		if (!ozz::animation::offline::fbx::ExtractAnimation(animation_name.c_str(), *scene_loader, *skeleton, 0.0f, &animation))
			throw std::runtime_error("Error extracting animation from " + path.string());

		if (is_additive)
		{
			ozz::animation::offline::AdditiveAnimationBuilder additive_builder;
			RawAnimation raw_additive;

			const bool succeeded = additive_builder(animation, &raw_additive);
			if (!succeeded)
				throw std::runtime_error("Error building additive animation " + path.string());

			animation = raw_additive;
		}

		ozz::animation::offline::AnimationOptimizer optimizer;
		RawAnimation raw_optimized_animation;
		if (!optimizer(animation, *skeleton, &raw_optimized_animation))
			throw std::runtime_error("Error optimizing animation " + std::string(animation_name.c_str()) + ", " + path.string());
		
		// Builds runtime animation.
		ozz::unique_ptr<ozz::animation::Animation> runtime_animation;
		AnimationBuilder builder;
		runtime_animation = builder(animation);
		if (!runtime_animation)
			throw std::runtime_error("Failed to build runtime animation " + std::string(animation_name.c_str()) + ", " + path.string());

		const auto output_path = GetAnimationOutputPath(path);
		ozz::string filename = output_path.string().c_str();
		ozz::io::File file(filename.c_str(), "wb");
		if (!file.opened())
			throw std::runtime_error("Failed to open output file: \"" + std::string(filename.c_str()) + "\"");

		// Initializes output archive.
		ozz::io::OArchive archive(&file, ozz::Endianness::kLittleEndian);
		archive << *runtime_animation;

		ozz::Delete(scene_loader);

		if (exported_assets)
			exported_assets->filenames.push_back(output_path);

		return true;
	}

	bool SceneExporter::ExportRootNode(FbxScene* scene, const std::filesystem::path& path, ExportedSceneAssets* exported_assets)
	{
		try
		{
			auto metadata = GetMetadataForFile(path);

			auto filename = path.filename().replace_extension().string();
			utils::Lowercase(filename);

			ozz::unique_ptr<Skeleton> skeleton;
			const bool has_skeleton = ExtractOzzRuntimeSkeleton(scene, scene->GetRootNode(), skeleton);
			if (has_skeleton)
			{
				auto skeleton_path = GetSkeletonOutputPath(path);
				if (!WriteSkeletonToFile(skeleton_path, *skeleton))
					throw std::runtime_error("Error exporting skeleton for " + path.string());
				exported_assets->filenames.push_back(skeleton_path);
			}

			auto meshes = GetMeshesToExport(scene);
			for (auto& it : meshes)
			{
				if (it.first == filename)
					throw std::runtime_error("Mesh can't have the same name as source file: " + it.first);

				it.second.submeshes = ExtractMeshes(it.second.meshes, it.second.root_node, scene, skeleton.get());
			}

			for (auto& it : meshes)
			{
				switch (it.second.type)
				{
					case MeshExportData::Type::Mesh:
					{
						auto output_filename = GetMeshOutputPath(it.first, path);
						if (!WriteMeshToFile(it.second.submeshes, output_filename))
							throw std::runtime_error("error writing mesh file: " + utils::WStringToString(output_filename));

						if (exported_assets)
							exported_assets->filenames.push_back(output_filename);
						break;
					}
					case MeshExportData::Type::PhysConvex:
					case MeshExportData::Type::Phys:
					{
						auto output_filename = fs::path(GetMeshOutputPath(it.first, path));
						output_filename.replace_extension(phys_extension);
						const bool convex = it.second.type == MeshExportData::Type::PhysConvex;
						if (!WritePhysMeshToFile(it.second.submeshes, output_filename, convex, cooking.get()))
							throw std::runtime_error("error writing phys file: " + utils::WStringToString(output_filename));

						if (exported_assets)
							exported_assets->filenames.push_back(output_filename);
						break;
					}
				}

			}
		}
		catch (std::runtime_error e)
		{
			std::cout << "Export error: " << e.what() << std::endl;
			return false;
		}

		return true;
	}

	bool SceneExporter::WriteSkeletonToFile(const std::filesystem::path& path, ozz::animation::Skeleton& skeleton)
	{
		ozz::io::File file(path.string().c_str(), "wb");
		if (!file.opened()) {
			throw std::runtime_error("Failed to open output file: \"" + path.string());
		}

		ozz::io::OArchive archive(&file, ozz::Endianness::kLittleEndian);
		archive << skeleton;

		return true;
	}

	fs::path SceneExporter::GetOutputPath(const fs::path& src_path)
	{
		if (input_is_in_assets)
		{
			auto path = src_path.wstring().replace(0, settings.assets_path.wstring().size(), settings.bin_assets_path );
			return path;
		}
		else 
			return src_path;
	}

	std::wstring SceneExporter::GetMeshOutputPath(const std::string& mesh_name, const fs::path& fbx_path)
	{
		auto result = fbx_path;
		if (mesh_name == default_key)
		{
			result.replace_extension(mesh_extension);
		}
		else
		{
			result = result.parent_path();
			result /= mesh_name;
			result.replace_extension(mesh_extension);
		}

		return GetOutputPath(result);
	}

	std::wstring SceneExporter::GetSkeletonOutputPath(const std::filesystem::path& fbx_path)
	{
		auto result = fbx_path;
		result.replace_extension(skeleton_extension);
		return GetOutputPath(result);
	}

	std::filesystem::path SceneExporter::GetAnimationOutputPath(const std::filesystem::path& fbx_path)
	{
		auto result = fbx_path;
		result.replace_extension(animation_extension);
		return GetOutputPath(std::move(result));
	}
}
