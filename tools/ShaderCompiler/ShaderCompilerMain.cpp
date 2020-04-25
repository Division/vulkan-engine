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
#include <mutex>
#include <algorithm>

const auto vs_extension = L".vert";
const auto fs_extension = L".frag";
const auto cs_extension = L".comp";
const auto hlsl_extension = L".hlsl";
const auto shaders_path = L"shaders";
const auto compiler_shaders_path = L"shaders_compiled";
const auto material_shader_name = L"material_main";
const auto compiler_path = "glslangValidator.exe";
const auto hlsl_compiler_path = "dxc.exe";

using namespace core::Device;

struct ShaderCompilerData
{
	ShaderProgram::Stage stage;
	std::filesystem::path path;
	bool is_hlsl = false;
};

const std::map<std::wstring, ShaderProgram::Stage> extension_to_stage = {
	{ vs_extension, ShaderProgram::Stage::Vertex },
	{ fs_extension, ShaderProgram::Stage::Fragment },
	{ cs_extension, ShaderProgram::Stage::Compute },
};

const std::map<ShaderProgram::Stage, std::string> stage_to_target = {
	{ ShaderProgram::Stage::Vertex, "vs_6_0" },
	{ ShaderProgram::Stage::Fragment, "ps_6_0" },
	{ ShaderProgram::Stage::Compute, "cs_6_0" },
};

struct CompilerSettings
{
	std::filesystem::path input_path;
	std::filesystem::path output_path;
	uint32_t max_threads = -1;
};

std::mutex output_mutex;
std::atomic_bool should_abort = false;

std::wstringstream log_output;

void CompileHLSLShader(const CompilerSettings& settings, const std::filesystem::path& absolute_input_path, const ShaderProgramInfo::ShaderData& data)
{
	auto hash_path = ShaderCache::GetShaderCachePath(ShaderCache::GetShaderDataHash(data));
	auto absolute_output_path = settings.output_path / hash_path;

	std::stringstream stream;
	stream << hlsl_compiler_path;
	stream << " -E " << data.entry_point;
	stream << " -T " << stage_to_target.at(data.stage);
	stream << " -Zi"; // debug info
	stream << " -spirv -fvk-use-gl-layout";
	for (auto& m : data.defines)
		stream << " -D" << m.name << "=" << m.value;
	stream << " -Fo " << absolute_output_path;
	stream << " " << absolute_input_path;

	{
		std::scoped_lock lock(output_mutex);
		log_output << "=======================================\n    [NAME]:    " << data.path << " (entry function: " << data.entry_point.c_str() << ")\n    [DEFINES]: ";
		for (auto& m : data.defines)
			log_output << m.name.c_str() << "=" << m.value.c_str() << ", ";
		if (data.defines.empty()) std::cout << "-";
		log_output << "\n    [HASH]:    " << hash_path << "\n    ";
	}

	if (std::system(stream.str().c_str()) != 0)
	{
		should_abort = true;
		throw std::runtime_error("error exporting shader");
	}
}

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

	{
		std::scoped_lock lock(output_mutex);
		log_output << "=======================================\n    [NAME]:    " << data.path << "\n    [DEFINES]: ";
		for (auto& m : data.defines)
			log_output << m.name.c_str() << "=" << m.value.c_str() << ", ";
		log_output << "\n    [HASH]:    " << hash_path << "\n    ";
	}

	if (std::system(stream.str().c_str()) != 0)
	{
		should_abort = true;
		throw std::runtime_error("error exporting shader");
	}
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
		("max-threads", "Maximum number of processing threads", cxxopts::value<uint32_t>())
		("output-path", "Shader output folder", cxxopts::value<std::string>())
		("input-path", "Shader input folder", cxxopts::value<std::string>());

	
	auto working_dir = std::filesystem::current_path();

	CompilerSettings settings;
	try {
		cxxopts::ParseResult parsed_options = options.parse(argc, argv);
		settings.output_path = parsed_options["output-path"].count() ? parsed_options["output-path"].as<std::string>() : working_dir;
		settings.input_path = parsed_options["input-path"].count() ? parsed_options["input-path"].as<std::string>() : settings.output_path / compiler_shaders_path;
		settings.max_threads = parsed_options["max-threads"].count() ? parsed_options["max-threads"].as<uint32_t>() : -1;
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
			if (ext == hlsl_extension)
			{
				shaders.push_back({ ShaderProgram::Stage::Vertex, path.path(), true });
			}
			else if (ext_iterator != extension_to_stage.end())
			{
				shaders.push_back({ ext_iterator->second, path.path(), false });
			}
		}
	}
	catch (std::runtime_error e)
	{
		std::cerr << "Error getting input shader list. " << e.what() << std::endl;
		return 1;
	}

	if (settings.max_threads == 0)
	{
		std::cerr << "max-threads can't be 0" << std::endl;
		return 1;
	}

	struct CompileData
	{
		ShaderProgramInfo::ShaderData shader_data;
		std::filesystem::path path;
		bool is_hlsl;
	};

	std::vector<CompileData> compile_infos;
	ShaderCapsSet depth_skinning_caps;
	depth_skinning_caps.addCap(ShaderCaps::Skinning);
	std::vector<ShaderProgramInfo::Macro> depth_skinning_defines = {{ "DEPTH_ONLY", "1" }};
	ShaderCache::AppendCapsDefines(depth_skinning_caps, depth_skinning_defines);

	for (auto& shader : shaders)
	{
		auto absolute_path = std::filesystem::canonical(shader.path);
		auto path = std::filesystem::proximate(std::filesystem::relative(absolute_path, std::filesystem::canonical(settings.input_path.parent_path()))).wstring();
		utils::ReplaceAll(path, L"\\", L"/" );
		bool is_material = shader.path.wstring().find(material_shader_name) != std::string::npos;

		if (shader.is_hlsl)
		{
			if (is_material)
			{
				ForEachCapsPermutation([&](ShaderCapsSet set) {
					std::vector<ShaderProgramInfo::Macro> defines;
					ShaderCache::AppendCapsDefines(set, defines);
					compile_infos.push_back({ ShaderProgramInfo::ShaderData(ShaderProgram::Stage::Vertex, path, "vs_main", defines), absolute_path, true});
					compile_infos.push_back({ ShaderProgramInfo::ShaderData(ShaderProgram::Stage::Fragment, path, "ps_main", defines), absolute_path, true});
				});

				if (shader.stage == ShaderProgram::Stage::Vertex)
				{
					compile_infos.push_back({ ShaderProgramInfo::ShaderData(ShaderProgram::Stage::Vertex, path, "vs_main", std::vector<ShaderProgramInfo::Macro>{{ "DEPTH_ONLY", "1" }}), absolute_path, true});
					compile_infos.push_back({ ShaderProgramInfo::ShaderData(ShaderProgram::Stage::Vertex, path, "vs_main", depth_skinning_defines), absolute_path, true });
				}
			}
			else {
				compile_infos.push_back({ ShaderProgramInfo::ShaderData(ShaderProgram::Stage::Vertex, path, "vs_main"), absolute_path, true});
				compile_infos.push_back({ ShaderProgramInfo::ShaderData(ShaderProgram::Stage::Fragment, path, "ps_main"), absolute_path, true});
			}
		}
		else
		{
			if (is_material)
			{
				ForEachCapsPermutation([&](ShaderCapsSet set) {
					std::vector<ShaderProgramInfo::Macro> defines;
					ShaderCache::AppendCapsDefines(set, defines);
					compile_infos.push_back({ ShaderProgramInfo::ShaderData(shader.stage, path, "main", defines), absolute_path, false});
				});

				if (shader.stage == ShaderProgram::Stage::Vertex)
				{
					compile_infos.push_back({ ShaderProgramInfo::ShaderData(shader.stage, path, "main", std::vector<ShaderProgramInfo::Macro>{{ "DEPTH_ONLY", "1" }}), absolute_path, false});
					compile_infos.push_back({ ShaderProgramInfo::ShaderData(shader.stage, path, "main", depth_skinning_defines), absolute_path, false});
				}
			}
			else {
				compile_infos.push_back({ ShaderProgramInfo::ShaderData(shader.stage, path), absolute_path, false});
			}
		}
	}

	auto compile_func = [&settings](CompileData& data) {
		if (data.is_hlsl)
			CompileHLSLShader(settings, data.path, data.shader_data);
		else
			CompileShader(settings, data.path, data.shader_data);
	};

	auto thread_func = [&compile_func](auto begin, auto end)
	{
		for (auto it = begin; it != end; it++)
		{
			if (should_abort)
				return;

			compile_func(*it);
		}
	};

	uint32_t thread_count = std::min(std::min(settings.max_threads, std::max(std::thread::hardware_concurrency(), 2u)), (uint32_t)compile_infos.size());
	std::vector<std::thread> threads;
	uint32_t per_thread = compile_infos.size() / thread_count;

	std::cout << "[THREADS:] " << thread_count << std::endl;

	for (int i = 0; i < compile_infos.size(); i += per_thread)
	{
		auto begin = compile_infos.begin() + i;
		auto end = compile_infos.begin() + std::min(i + per_thread, (uint32_t)compile_infos.size());
		threads.emplace_back(thread_func, begin, end);
	}

	for (auto& thread : threads)
		thread.join();

	std::wcout << log_output.str();

	return 0;
}