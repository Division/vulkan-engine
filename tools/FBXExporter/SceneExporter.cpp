#include <iostream>
#include "SceneExporter.h"
#include "utils/StringUtils.h"
#include "SceneExporterUtils.h"
#include <fbxsdk/utils/fbxgeometryconverter.h>

namespace fs = std::filesystem;

using namespace physx;
using namespace Physics;

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

	SceneExporter::SceneExporter(FBXExporterSetting settings) : settings(settings)
	{
		manager = FbxManager::Create();
		FbxIOSettings *ios = FbxIOSettings::Create(manager, IOSROOT);
		manager->SetIOSettings(ios);
		input_is_in_assets = utils::BeginsWith(settings.input_path.wstring(), settings.assets_path.wstring());

		static PxDefaultAllocator DefaultAllocator;
		static ErrorCallback ErrorCallback;

		foundation = Handle(PxCreateFoundation(PX_PHYSICS_VERSION, DefaultAllocator, ErrorCallback));
		if (!foundation)
			throw std::runtime_error("PxCreateFoundation failed!");

		PxTolerancesScale scale;
		cooking = PxCreateCooking(PX_PHYSICS_VERSION, *foundation, PxCookingParams(scale));
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
			std::cout << "Directories not supported yet.\n";
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

	bool SceneExporter::ExportFBXFile(const std::filesystem::path& path)
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

		FbxGeometryConverter converter(manager);
		if (!converter.Triangulate(scene, true))
			std::wcout << L"[Warning] triangulation finished with errors.\n";

		if (!scene->GetRootNode())
		{
			std::wcout << L"Scene doesn't have root node" << std::endl;
			return false;
		}

		return ExportRootNode(scene, path);
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
					result[current_name].meshes.push_back({(FbxMesh*)attrib});
				}
				if (attrib->GetAttributeType() == FbxNodeAttribute::eSkeleton)
				{
					result[current_name].bones.push_back((FbxSkeleton*)attrib);
				}
			}

			return true;
		});

		return result;
	}

	bool SceneExporter::ExportRootNode(FbxScene* scene, const std::filesystem::path& path)
	{
		try
		{
			auto metadata = GetMetadataForFile(path);

			auto filename = path.filename().replace_extension().string();
			utils::Lowercase(filename);

			auto meshes = GetMeshesToExport(scene);
			for (auto& it : meshes)
			{
				if (it.first == filename)
					throw std::runtime_error("Mesh can't have the same name as source file: " + it.first);

				it.second.submeshes = ExtractMeshes(it.second.meshes);
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

	fs::path SceneExporter::GetOutputPath(const fs::path& src_path)
	{
		if (input_is_in_assets)
		{
			auto path = src_path.wstring().replace(0, settings.assets_path.wstring().size(), settings.bin_assets_path);
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
}
