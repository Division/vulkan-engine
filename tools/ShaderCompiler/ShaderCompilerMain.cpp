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
#include <thread>
#include <algorithm>

const auto vs_extension = L".vert";
const auto fs_extension = L".frag";
const auto cs_extension = L".comp";
const auto shaders_path = L"shaders";
const auto compiler_shaders_path = L"shaders_compiled";
const auto material_shader_name = L"material_main";
const auto compiler_path = "glslangValidator.exe";

using namespace core::Device;

struct ShaderCompilerData
{
	ShaderProgram::Stage stage;
	std::filesystem::path path;
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

void CompileShader(const CompilerSettings& settings, const std::filesystem::path& absolute_input_path, const ShaderProgramInfo::ShaderData& data)
{
	auto hash_path = ShaderCache::GetShaderCachePath(ShaderCache::GetShaderDataHash(data));
	auto absolute_output_path = settings.output_path / hash_path;

	std::stringstream stream;
	stream << compiler_path;
	stream << " -V";
	for (auto& m : data.defines)
		stream << " -D" << m.name;
	stream << " -o " << absolute_output_path;
	stream << " " << absolute_input_path;

	//std::cout << stream.str().c_str() << std::endl;

	std::wcout << "=======================================\n    [NAME]:    " << data.path << "\n    [DEFINES]: ";
	for (auto& m : data.defines)
		std::cout << m.name << "=" << m.value << ", ";
	std::wcout << "\n    [HASH]:    " << hash_path << "\n    ";

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
	std::vector<ShaderCompilerData> shaders;
	
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
				shaders.push_back({ ext_iterator->second, file_path });
			}
		}
	}
	catch (std::runtime_error e)
	{
		std::cerr << "Error getting input shader list. " << e.what() << std::endl;
		return 1;
	}

	std::vector<std::pair<ShaderProgramInfo::ShaderData, std::filesystem::path>> compile_infos;
	ShaderCapsSet depth_skinning_caps;
	depth_skinning_caps.addCap(ShaderCaps::Skinning);
	std::vector<ShaderProgramInfo::Macro> depth_skinning_defines = {{ "DEPTH_ONLY", "1" }};
	ShaderCache::AppendCapsDefines(depth_skinning_caps, depth_skinning_defines);

	for (auto& shader : shaders)
	{
		auto absolute_path = std::filesystem::canonical(shader.path);
		auto path = std::filesystem::proximate(std::filesystem::relative(absolute_path, std::filesystem::canonical(settings.input_path.parent_path()))).wstring();
		utils::ReplaceAll(path, L"\\", L"/" );

		if (shader.path.wstring().find(material_shader_name) != std::string::npos)
		{
			ForEachCapsPermutation([&](ShaderCapsSet set) {
				std::vector<ShaderProgramInfo::Macro> defines;
				ShaderCache::AppendCapsDefines(set, defines);
				compile_infos.emplace_back(std::make_pair(ShaderProgramInfo::ShaderData(shader.stage, path, "main", defines), absolute_path));
			});

			if (shader.stage == ShaderProgram::Stage::Vertex)
			{
				compile_infos.emplace_back(std::make_pair(ShaderProgramInfo::ShaderData(shader.stage, path, "main", std::vector<ShaderProgramInfo::Macro>{{ "DEPTH_ONLY", "1" }}), absolute_path));
				compile_infos.emplace_back(std::make_pair(ShaderProgramInfo::ShaderData(shader.stage, path, "main", depth_skinning_defines), absolute_path));
			}
		}
		else {
			compile_infos.emplace_back(std::make_pair(ShaderProgramInfo::ShaderData(shader.stage, path), absolute_path));
		}
	}

	for (auto& info : compile_infos)
	{
		CompileShader(settings, info.second, info.first);
	}

	return 0;
}