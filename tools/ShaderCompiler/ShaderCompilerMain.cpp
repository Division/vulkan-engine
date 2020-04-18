#include "CommonIncludes.h"
#include <cxxopts/cxxopts.hpp>
#include <cstdlib>
#include <string>
#include "render/shader/Shader.h"
#include "render/shader/ShaderDefines.h"
#include "render/shader/ShaderCaps.h"
#include "render/shader/ShaderCache.h"
#include "utils/Math.h"
#include "utils/StringUtils.h"

const auto vs_extension = L".vert";
const auto fs_extension = L".frag";
const auto cs_extension = L".comp";
const auto shaders_path = L"shaders";
const auto compiler_shaders_path = L"shaders_compiled";
const auto material_shader_name = L"material_main";
const auto compiler_path = "glslangValidator.exe";

using namespace core::Device;

struct ShaderData
{
	ShaderProgram::Stage stage;
	std::filesystem::path path;
	std::filesystem::path path_no_extension;
};

const std::map<std::wstring, ShaderProgram::Stage> extension_to_stage = {
	{ vs_extension, ShaderProgram::Stage::Vertex },
	{ fs_extension, ShaderProgram::Stage::Fragment },
	{ cs_extension, ShaderProgram::Stage::Compute },
};

struct CompilerSettings
{
	std::filesystem::path input_path;
	std::filesystem::path output_path;
};

void CompileShader(const CompilerSettings& settings, const std::filesystem::path& shader_path, const ShaderCapsSet& set, bool is_material, std::vector<std::string> additional_macro = {})
{
	auto hash_src_path = std::filesystem::proximate(std::filesystem::relative(std::filesystem::canonical(shader_path), std::filesystem::canonical(settings.input_path.parent_path()))).wstring();
	utils::ReplaceAll(hash_src_path, L"\\", L"/" );
	auto relative_shader_path = std::filesystem::relative(std::filesystem::canonical(shader_path));

	std::vector<std::string> macro;
	for (int i = 0; i < (int)ShaderCaps::Count; i++)
		if (set.hasCap((ShaderCaps)i))
			macro.push_back(SHADER_CAPS_DEFINES.at((ShaderCaps)i));

	for (auto& m : additional_macro)
		macro.push_back(m);

	auto relative_shader_path_string = relative_shader_path.wstring();

	auto hash_path = is_material 
		? ShaderCache::GetMaterialShaderCachePath(hash_src_path, set, additional_macro) 
		: ShaderCache::GetShaderCachePath(ShaderCache::GetShaderPathHash(hash_src_path, additional_macro));

	auto absolute_output_path = settings.output_path / hash_path;
	std::filesystem::path absolute_input_path = std::filesystem::current_path() / relative_shader_path;
	std::stringstream stream;
	stream << compiler_path;
	stream << " -V";
	for (auto& m : macro)
		stream << " -D" << m;
	stream << " -o " << absolute_output_path;
	stream << " " << absolute_input_path;

	//std::cout << stream.str().c_str() << std::endl;

	std::wcout << "    [HASHED NAME]: " << hash_src_path << "  (is_material: " << is_material << ", bitmask: " << set.getBitmask() <<  ") :\n      => " << hash_path << std::endl;

	if (std::system(stream.str().c_str()) != 0)
		throw std::runtime_error("error exporting shader");
}

void ForEachCapsPermutation(std::function<void(ShaderCapsSet)> callback)
{
	std::vector<ShaderCaps> caps_to_iterate = { ShaderCaps::Skinning, ShaderCaps::Lighting, ShaderCaps::NormalMap, ShaderCaps::Texture0, ShaderCaps::Color };
	for (int i = 0; i < caps_to_iterate.size() * caps_to_iterate.size(); i++)
	{
		ShaderCapsSet caps_set;
		for (int j = 0; j < caps_to_iterate.size(); j++)
			if (i & (1 << j))
				caps_set.addCap(caps_to_iterate[j]);
		callback(caps_set);
	}
}

int main(int argc, char* argv[]) 
{
	cxxopts::Options options("Shader compiler", "compiles glsl to SPIR-V");
	options.add_options()
		("output-path", "Shader output folder", cxxopts::value<std::string>())
		("input-path", "Shader input folder", cxxopts::value<std::string>());

	
	auto working_dir = std::filesystem::current_path();

	CompilerSettings settings;
	try {
		cxxopts::ParseResult parsed_options = options.parse(argc, argv);
		settings.output_path = parsed_options["output-path"].count() ? parsed_options["output-path"].as<std::string>() : working_dir;
		settings.input_path = parsed_options["input-path"].count() ? parsed_options["input-path"].as<std::string>() : settings.output_path / compiler_shaders_path;
	}
	catch (cxxopts::OptionParseException e)
	{
		std::cerr << "Invalid arguments. " << e.what() << std::endl;
		return 1;
	}

	try
	{
		std::cout << "[Shader Compiler]\nWorking directory: " << working_dir << std::endl;
		std::cout << "Input path: " << std::filesystem::absolute(settings.input_path) << std::endl;
		std::error_code ec;
		std::cout << "Output path: " << std::filesystem::absolute(settings.output_path, ec) << std::endl;
	}
	catch (std::runtime_error e)
	{
		std::cerr << "Invalid input. " << e.what() << std::endl;
		return 1;
	}

	std::filesystem::create_directories(settings.output_path);
	std::vector<ShaderData> shaders;
	
	try
	{
		std::filesystem::recursive_directory_iterator iter(settings.input_path);
		for (auto& path : iter)
		{
			auto ext = path.path().extension();
			auto ext_iterator = extension_to_stage.find(ext);
			if (ext_iterator != extension_to_stage.end())
			{
				auto file_path = path.path();
				shaders.push_back({ ext_iterator->second, file_path, file_path.replace_extension("") });
			}
		}
	}
	catch (std::runtime_error e)
	{
		std::cerr << "Error getting input shader list. " << e.what() << std::endl;
		return 1;
	}

	for (auto& shader : shaders)
	{
		if (shader.path.wstring().find(material_shader_name) != std::string::npos)
		{
			ForEachCapsPermutation([&](ShaderCapsSet set) {
				CompileShader(settings, shader.path, set, true);
			});

			if (shader.stage == ShaderProgram::Stage::Vertex)
			{
				ShaderCapsSet depth_caps;
				CompileShader(settings, shader.path, depth_caps, true, { "DEPTH_ONLY" });
				depth_caps.addCap(ShaderCaps::Skinning);
				CompileShader(settings, shader.path, depth_caps, true, { "DEPTH_ONLY" });
			}
		}
		else {
			CompileShader(settings, shader.path, ShaderCapsSet(), false);
		}

	}

	return 0;
}