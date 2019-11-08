#pragma once

#include "CommonIncludes.h"
#include "ShaderCaps.h"

namespace core { namespace Device {

	class ShaderModule;
	class ShaderProgram;

	class ShaderCache
	{
	public:
		static uint32_t GetDefinesHash(const std::vector<std::string>& defines);
		static uint32_t GetCombinedHash(uint32_t name_hash, const ShaderCapsSet& caps_set);
		static uint32_t GetShaderPathHash(const std::wstring& path, const std::vector<std::string>& defines = {});
		static uint32_t GetMaterialShaderHash(const std::wstring& path, const ShaderCapsSet& caps_set, const std::vector<std::string>& defines = {});
		static std::wstring GetMaterialShaderCachePath(const std::wstring& path, const ShaderCapsSet& caps_set, const std::vector<std::string>& defines = {});
		static std::wstring GetMaterialShaderCachePath(uint32_t name_hash, const ShaderCapsSet& caps_set);
		static std::wstring GetShaderCachePath(uint32_t shader_hash, const std::vector<std::string>& defines = {});
		static const wchar_t* shader_cache_dir;

	public:
		ShaderCache();
		~ShaderCache();
		ShaderProgram* GetShaderProgram(uint32_t vertex_hash, uint32_t fragment_hash);
		ShaderModule* GetShaderModule(uint32_t hash);

	private:
		std::unordered_map<uint32_t, std::unique_ptr<ShaderModule>> module_cache;
		std::unordered_map<uint32_t, std::unique_ptr<ShaderProgram>> program_cache;
			

	};

} }