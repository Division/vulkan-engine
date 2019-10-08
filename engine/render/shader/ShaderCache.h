#pragma once

#include "CommonIncludes.h"
#include "ShaderCaps.h"

namespace core { namespace Device {

	class ShaderCache
	{
	public:
		static uint32_t GetMaterialShaderHash(std::wstring& path, const ShaderCapsSet& caps_set);
		static std::wstring GetMaterialShaderCachePath(std::wstring& path, const ShaderCapsSet& caps_set);
		static std::wstring GetMaterialShaderCachePath(uint32_t name_hash, const ShaderCapsSet& caps_set);

		static const wchar_t* shader_cache_dir;
	};

} }