#include <cxxopts/cxxopts.hpp>
#include "SceneExporter.h"
#include "FBXExporterSettings.h"

int main(int argc, char** argv) {

    Exporter::FBXExporterSetting settings;

    if (argc == 0)
    {
        std::cout << "Not enough arguments";
        return 1;
    }

    std::string path = argv[0];

    settings.bin_path = std::filesystem::path(path).parent_path();
    settings.bin_assets_path = settings.bin_path / "assets";
    settings.assets_path = settings.bin_path.parent_path() / "assets";
    std::filesystem::create_directories(settings.bin_assets_path);

#if defined (_DEBUG)
    //settings.input_path = settings.assets_path / "Models/test/frog.fbx";
    settings.input_path = settings.assets_path / "Models/Turret/Turret.fbx";
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