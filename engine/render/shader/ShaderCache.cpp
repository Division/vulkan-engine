#include "ShaderCache.h"
#include "utils/Math.h"

namespace core { namespace Device {

	const wchar_t* ShaderCache::shader_cache_dir = L"shaders_compiled";

	uint32_t ShaderCache::GetMaterialShaderHash(std::wstring& path, const ShaderCapsSet& caps_set)
	{
		uint32_t combined_hash[] = { FastHash(path.data(), path.length() * sizeof(wchar_t)), caps_set.getBitmask() };
		return FastHash(combined_hash, sizeof(combined_hash));
	}

	std::wstring ShaderCache::GetMaterialShaderCachePath(std::wstring& path, const ShaderCapsSet& caps_set)
	{
		return GetMaterialShaderCachePath(FastHash(path.data(), path.length() * sizeof(wchar_t)), caps_set);
	}

	std::wstring ShaderCache::GetMaterialShaderCachePath(uint32_t name_hash, const ShaderCapsSet& caps_set)
	{
		uint32_t combined_hash[] = { name_hash, caps_set.getBitmask() };
		auto hash = FastHash(combined_hash, sizeof(combined_hash));
		std::filesystem::path shader_path(shader_cache_dir);
		
		std::wstring filename = std::to_wstring(hash);

		shader_path /= filename;
		shader_path.replace_extension(".spv");
		return shader_path.wstring();
	}

} }