#include <cstdlib>
#include "CommonIncludes.h"
#include "render/shader/Shader.h"
#include "render/shader/ShaderDefines.h"
#include "render/shader/ShaderCaps.h"
#include "render/shader/ShaderCache.h"
#include "utils/Math.h"
#include "utils/StringUtils.h"

const auto vs_extension = L".vert";
const auto fs_extension = L".frag";
const auto shaders_path = L"shaders";
const auto compiler_shaders_path = L"shaders_compiled";
const auto material_shader_name = L"material_main";
const auto compiler_path = "glslangValidator.exe";

using namespace core::Device;

struct ShaderData
{
	ShaderProgram::Stage stage;
	std::filesystem::path path;
};

void CompileShader(const std::filesystem::path& shader_path, const ShaderCapsSet& set, std::vector<std::string> additional_macro = {})
{

	auto relative_shader_path = std::filesystem::relative(std::filesystem::canonical(shader_path));

	std::vector<std::string> macro;
	for (int i = 0; i < (int)ShaderCaps::Count; i++)
		if (set.hasCap((ShaderCaps)i))
			macro.push_back(SHADER_CAPS_DEFINES.at((ShaderCaps)i));

	//std::for_each(additional_macro.begin(), additional_macro.end(), std::back_inserter(macro)); // TODO: calculate hash with defines

	auto relative_shader_path_string = relative_shader_path.wstring();
	utils::ReplaceAll(relative_shader_path_string, L"\\", L"/" );

	auto absolute_output_path = std::filesystem::current_path() / ShaderCache::GetMaterialShaderCachePath(relative_shader_path_string, set);
	std::filesystem::path absolute_input_path = std::filesystem::current_path() / relative_shader_path;
	std::stringstream stream;
	stream << compiler_path;
	stream << " -V";
	for (auto& m : macro)
		stream << " -D" << m;
	stream << " -o " << absolute_output_path;
	stream << " " << absolute_input_path;

	//std::cout << stream.str().c_str() << std::endl;

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
		if (ext == vs_extension || ext == fs_extension)
		{
			shaders.push_back({ {}, path.path() });
		}
	}

	for (auto& shader : shaders)
	{
		if (shader.path.wstring().find(material_shader_name) != std::string::npos)
		{
			ForEachCapsPermutation([&](ShaderCapsSet set) {
				CompileShader(shader.path, set);
			});
		}
		else {
			CompileShader(shader.path, ShaderCapsSet());
		}

	}

	return 0;
}