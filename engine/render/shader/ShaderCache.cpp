#include "ShaderCache.h"
#include "utils/Math.h"
#include "loader/FileLoader.h"
#include "render/shader/ShaderDefines.h"

namespace Device {

	const wchar_t* ShaderCache::shader_cache_dir = L"shaders_compiled";

	ShaderProgramInfo& ShaderProgramInfo::AddShader(
		ShaderProgram::Stage stage, std::wstring path, std::string entry_point, std::vector<ShaderProgramInfo::Macro> defines
	)
	{
		shaders.push_back({ stage, path, std::move(entry_point), std::move(defines) });
		return *this;
	}

	void ShaderCache::AppendCapsDefines(const ShaderCapsSet& caps_set, std::vector<ShaderProgramInfo::Macro>& out_defines)
	{
		if (caps_set.getBitmask() == 0) return;

		for (int i = 0; i < (int)ShaderCaps::Count; i++)
			if (caps_set.hasCap((ShaderCaps)i))
				out_defines.push_back({ SHADER_CAPS_DEFINES.at((ShaderCaps)i), "1"});
	}

	uint32_t ShaderCache::GetDefinesHash(std::vector<ShaderProgramInfo::Macro> defines)
	{
		uint32_t hash = 0;
		if (defines.empty()) return hash;

		std::sort(defines.begin(), defines.end(), [](const ShaderProgramInfo::Macro& a, const ShaderProgramInfo::Macro& b) { return a.name < b.name; });

		for (int i = 0; i < defines.size(); i++)
		{
			auto& define = defines[i];
			auto name_hash = define.name.length() ? FastHash(define.name.data(), define.name.length() * sizeof(char)) : 0u;
			auto value_hash = define.value.length() ? FastHash(define.value.data(), define.value.length() * sizeof(char)) : 0u;
			uint32_t combined[] = { hash, name_hash, value_hash };
			hash = FastHash(combined, sizeof(combined));
		}

		return hash;
	}

	uint32_t ShaderCache::GetCombinedHash(uint32_t name_hash, const ShaderCapsSet& caps_set)
	{
		uint32_t combined_hash[] = { name_hash, caps_set.getBitmask() };
		return FastHash(combined_hash, sizeof(combined_hash));
	}

	uint32_t ShaderCache::GetShaderDataHash(const ShaderProgramInfo::ShaderData& data)
	{
		auto defines_hash = GetDefinesHash(data.defines);
		auto path_hash = FastHash(data.path.data(), data.path.length() * sizeof(wchar_t));
		auto entry_point_hash = FastHash(data.entry_point.data(), data.entry_point.length());
		uint32_t hashes[] = { defines_hash, path_hash, entry_point_hash };
		return FastHash(hashes, sizeof(hashes));
	}

	std::wstring ShaderCache::GetShaderCachePath(uint32_t shader_hash)
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

	ShaderProgram* ShaderCache::GetShaderProgram(const ShaderProgramInfo& info)
	{
		uint32_t vertex_hash = 0;
		uint32_t fragment_hash = 0;
		uint32_t compute_hash = 0;

		for (auto& data : info.shaders)
		{
			uint32_t hash = GetShaderDataHash(data);
			switch (data.stage)
			{
			case ShaderProgram::Stage::Vertex:
				vertex_hash = hash;
				break;
			case ShaderProgram::Stage::Fragment:
				fragment_hash = hash;
				break;
			case ShaderProgram::Stage::Compute:
				compute_hash = hash;
				break;
			default:
				throw std::runtime_error("unknown stage");
			}
		}

		return GetShaderProgram(vertex_hash, fragment_hash, compute_hash);
	}

	ShaderProgram* ShaderCache::GetShaderProgram(uint32_t vertex_hash, uint32_t fragment_hash, uint32_t compute_hash)
	{
		// TODO: async shader loading
		auto program_hash = ShaderProgram::CalculateHash(fragment_hash, vertex_hash, compute_hash);
		
		auto iter = program_cache.find(program_hash);
		if (iter != program_cache.end())
			return iter->second.get();

		auto program = std::make_unique<ShaderProgram>();
		if (vertex_hash)
		{
			auto vertex_module = GetShaderModule(vertex_hash);
			program->AddModule(vertex_module, ShaderProgram::Stage::Vertex);
		}
		if (fragment_hash)
		{
			auto fragment_module = GetShaderModule(fragment_hash);
			program->AddModule(fragment_module, ShaderProgram::Stage::Fragment);
		}
		if (compute_hash)
		{
			auto compute_module = GetShaderModule(compute_hash);
			program->AddModule(compute_module, ShaderProgram::Stage::Compute);
		}
		program->Prepare();
		auto* result = program.get();
		program_cache[program_hash] = std::move(program);
		return result;
	}


}