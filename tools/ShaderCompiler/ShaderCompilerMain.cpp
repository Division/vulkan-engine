#include <cstdlib>
#include "CommonIncludes.h"
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

void CompileShader(const std::filesystem::path& shader_path, const ShaderCapsSet& set, bool is_material, std::vector<std::string> additional_macro = {})
{
	auto relative_shader_path = std::filesystem::relative(std::filesystem::canonical(shader_path));

	std::vector<std::string> macro;
	for (int i = 0; i < (int)ShaderCaps::Count; i++)
		if (set.hasCap((ShaderCaps)i))
			macro.push_back(SHADER_CAPS_DEFINES.at((ShaderCaps)i));

	for (auto& m : additional_macro)
		macro.push_back(m);

	auto relative_shader_path_string = relative_shader_path.wstring();
	utils::ReplaceAll(relative_shader_path_string, L"\\", L"/" );

	auto hash_path = is_material 
		? ShaderCache::GetMaterialShaderCachePath(relative_shader_path_string, set, additional_macro) 
		: ShaderCache::GetShaderCachePath(ShaderCache::GetShaderPathHash(relative_shader_path_string, additional_macro));

	auto absolute_output_path = std::filesystem::current_path() / hash_path;
	std::filesystem::path absolute_input_path = std::filesystem::current_path() / relative_shader_path;
	std::stringstream stream;
	stream << compiler_path;
	stream << " -V";
	for (auto& m : macro)
		stream << " -D" << m;
	stream << " -o " << absolute_output_path;
	stream << " " << absolute_input_path;

	//std::cout << stream.str().c_str() << std::endl;

	std::wcout << shader_path << "\n         => " << hash_path << std::endl;

	if (std::system(stream.str().c_str()) != 0)
		throw std::runtime_error("error exporting shader");
}

void ForEachCapsPermutation(std::function<void(ShaderCapsSet)> callback)
{
	std::vector<ShaderCaps> caps_to_iterate = { ShaderCaps::Skinning, ShaderCaps::Lighting, ShaderCaps::NormalMap, ShaderCaps::Texture0 };
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
	auto working_dir = std::filesystem::current_path();
	std::cout << "working directory: " << working_dir << std::endl;

	auto shaders_dir = working_dir / shaders_path;
	std::filesystem::recursive_directory_iterator iter(shaders_dir);
	std::filesystem::recursive_directory_iterator end;
	
	std::vector<ShaderData> shaders;

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

	for (auto& shader : shaders)
	{
		if (shader.path.wstring().find(material_shader_name) != std::string::npos)
		{
			ForEachCapsPermutation([&](ShaderCapsSet set) {
				CompileShader(shader.path, set, true);
			});

			if (shader.stage == ShaderProgram::Stage::Vertex)
			{
				ShaderCapsSet depth_caps;
				CompileShader(shader.path, depth_caps, true, { "DEPTH_ONLY" });
				depth_caps.addCap(ShaderCaps::Skinning);
				CompileShader(shader.path, depth_caps, true, { "DEPTH_ONLY" });
			}
		}
		else {
			CompileShader(shader.path, ShaderCapsSet(), false);
		}

	}

	return 0;
}