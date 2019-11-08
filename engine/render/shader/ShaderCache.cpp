#include "ShaderCache.h"
#include "utils/Math.h"
#include "loader/FileLoader.h"
#include "Shader.h"

namespace core { namespace Device {

	const wchar_t* ShaderCache::shader_cache_dir = L"shaders_compiled";

	uint32_t ShaderCache::GetDefinesHash(const std::vector<std::string>& defines)
	{
		uint32_t hash = 0;
		for (int i = 0; i < defines.size(); i++)
		{
			auto& define = defines[i];
			auto define_hash = define.length() ? FastHash(define.data(), define.length() * sizeof(char)) : 0u;
			uint32_t combined[2] = { hash, define_hash };
			hash = FastHash(combined, sizeof(combined));
		}

		return hash;
	}

	uint32_t ShaderCache::GetCombinedHash(uint32_t name_hash, const ShaderCapsSet& caps_set)
	{
		uint32_t combined_hash[] = { name_hash, caps_set.getBitmask() };
		return FastHash(combined_hash, sizeof(combined_hash));
	}

	uint32_t ShaderCache::GetShaderPathHash(const std::wstring& path, const std::vector<std::string>& defines)
	{
		uint32_t combined_hash[] = { FastHash(path.data(), path.length() * sizeof(wchar_t)), GetDefinesHash(defines) };
		return FastHash(combined_hash, sizeof(combined_hash));
	}

	uint32_t ShaderCache::GetMaterialShaderHash(const std::wstring& path, const ShaderCapsSet& caps_set, const std::vector<std::string>& defines)
	{
		return GetCombinedHash(GetShaderPathHash(path, defines), caps_set);
	}

	std::wstring ShaderCache::GetMaterialShaderCachePath(const std::wstring& path, const ShaderCapsSet& caps_set, const std::vector<std::string>& defines)
	{
		return GetMaterialShaderCachePath(GetShaderPathHash(path, defines), caps_set);
	}

	std::wstring ShaderCache::GetMaterialShaderCachePath(uint32_t name_hash, const ShaderCapsSet& caps_set)
	{
		auto hash = GetCombinedHash(name_hash, caps_set);
		return GetShaderCachePath(hash);
	}

	std::wstring ShaderCache::GetShaderCachePath(uint32_t shader_hash, const std::vector<std::string>& defines)
	{
		std::filesystem::path shader_path(shader_cache_dir);

		std::wstring filename = std::to_wstring(shader_hash);

		shader_path /= filename;
		shader_path.replace_extension(".spv");
		return shader_path.wstring();
	}

	ShaderCache::ShaderCache() = default;
	ShaderCache::~ShaderCache() = default;

	ShaderModule* ShaderCache::GetShaderModule(uint32_t hash)
	{
		auto iter = module_cache.find(hash);
		if (iter != module_cache.end())
			return iter->second.get();

		std::wstring filename = ShaderCache::GetShaderCachePath(hash);

		auto module_data = loader::LoadFile(filename);
		if (!module_data.size())
			throw std::runtime_error("Error loading shader module");

		auto shader_module = std::make_unique<ShaderModule>(module_data.data(), module_data.size(), hash);
		auto* result = shader_module.get();
		module_cache[hash] = std::move(shader_module);

		return result;
	}

	ShaderProgram* ShaderCache::GetShaderProgram(uint32_t vertex_hash, uint32_t fragment_hash)
	{
		// TODO: async shader loading
		auto program_hash = ShaderProgram::CalculateHash(fragment_hash, vertex_hash);
		
		auto iter = program_cache.find(program_hash);
		if (iter != program_cache.end())
			return iter->second.get();

		auto program = std::make_unique<ShaderProgram>();
		auto vertex_module = GetShaderModule(vertex_hash);
		auto fragment_module = GetShaderModule(fragment_hash);
		program->AddModule(vertex_module, ShaderProgram::Stage::Vertex);
		program->AddModule(fragment_module, ShaderProgram::Stage::Fragment);
		program->Prepare();
		auto* result = program.get();
		program_cache[program_hash] = std::move(program);
		return result;
	}


} }