#include <fstream>
#include <cxxopts/cxxopts.hpp>
#include "SceneExporter.h"
#include "SceneExporterUtils.h"
#include "FBXExporterSettings.h"

namespace fs = std::filesystem;

int main(int argc, char** argv) {

    Exporter::FBXExporterSetting settings;
    std::string path = argv[0];
    settings.bin_path = std::filesystem::path(path).parent_path();

    if (fs::exists("fbx_exporter.json"))
    {
        rapidjson::Document json;
        if (!Exporter::LoadJSONFile("fbx_exporter.json", json) || !json.IsObject())
        {
            std::wcerr << "Error parsing fbx_exporter.json\n";
            return 1;
        }

        if (!json.HasMember("asset_path") || !json.HasMember("exported_asset_path"))
        {
            std::wcerr << "fbx_exporter.json should contain both \"asset_path\" and \"exported_asset_path\" keys\n";
            return 1;
        }

        settings.bin_assets_path = fs::absolute(fs::path(json["exported_asset_path"].GetString()));
        settings.assets_path = fs::absolute(fs::path(json["asset_path"].GetString()));
    }

    if (argc == 0)
    {
        std::wcerr << "Not enough arguments\n";
        return 1;
    }

    std::filesystem::create_directories(settings.bin_assets_path);

#if defined (_DEBUG)
    settings.input_path = settings.assets_path / "Entities/Insect/Insect_mesh.fbx";
    //settings.input_path = settings.assets_path / "Entities/Basic/figures.fbx";
    //settings.input_path = settings.assets_path / "Entities/Sphere/sphere.fbx";
    //settings.input_path = settings.assets_path / "Entities/Insect/animations/Insect@Flying.fbx";
#endif

    if (argc == 2)
        settings.input_path = argv[1];

    std::cout << "Input path: " << settings.input_path << std::endl;
    std::cout << "Assets output path: " << settings.bin_assets_path << std::endl;
    std::cout << "Assets base path: " << settings.assets_path << std::endl;

    Exporter::SceneExporter scene_exporter(settings);
    bool result = scene_exporter.Export();
    return result ? 0 : 1;
}