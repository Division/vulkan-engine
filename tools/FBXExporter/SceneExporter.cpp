#include <iostream>
#include "SceneExporter.h"
#include "utils/StringUtils.h"
#include "SceneExporterUtils.h"
#include <fbxsdk/utils/fbxgeometryconverter.h>

namespace fs = std::filesystem;

namespace Exporter
{
	namespace
	{
		const std::string mesh_prefix = "mesh_";
		const std::string default_key = "_default";
		const std::wstring mesh_extension = L".mesh";
	}

	SceneExporter::SceneExporter(FBXExporterSetting settings) : settings(settings)
	{
		manager = FbxManager::Create();
		FbxIOSettings *ios = FbxIOSettings::Create(manager, IOSROOT);
		manager->SetIOSettings(ios);
		input_is_in_assets = utils::BeginsWith(settings.input_path.wstring(), settings.assets_path.wstring());
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
					throw std::runtime_error("Nested meshes not supported");

				current_name = name.substr(mesh_prefix.size());
				utils::Lowercase(current_name);

				if (result.find(current_name) != result.end())
					throw std::runtime_error("Mesh already defined: " + current_name);

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
				auto output_filename = GetMeshOutputPath(it.first, path);
				if (!WriteMeshToFile(it.second.submeshes, output_filename))
					throw std::runtime_error("error writing mesh file: " + utils::WStringToString(output_filename));
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
