 #pragma once

#include "CommonIncludes.h"
#include "ShaderCaps.h"
#include "utils/DataStructures.h"
#include "Shader.h"

namespace Device {

	class ShaderModule;
	class ShaderProgram;

	struct ShaderProgramInfo
	{
		struct Macro
		{
			std::string name;
			std::string value;
		};

		struct ShaderData
		{
			ShaderProgram::Stage stage;
			std::wstring path;
			std::string entry_point;
			std::vector<Macro> defines;
			ShaderData& operator=(const ShaderData& other) = default;
			ShaderData() = default;
			ShaderData(const ShaderData& other) = default;
			ShaderData(ShaderData&& other) = default;
			ShaderData(ShaderProgram::Stage stage, std::wstring path, std::string entry_point = "main", std::vector<Macro> defines = {})
				: stage(stage), path(path), entry_point(entry_point), defines(std::move(defines)) {}
		};
		
		ShaderProgramInfo& AddShader(ShaderProgram::Stage stage, std::wstring path, std::string entry_point = "main", std::vector<Macro> defines = {});
		
		//utils::SmallVector<ShaderData, 3> shaders;
		std::vector<ShaderData> shaders;
	};

	class ShaderCache
	{
	public:
		static uint32_t GetShaderDataHash(const ShaderProgramInfo::ShaderData& data);
		static std::wstring GetShaderCachePath(uint32_t shader_hash);
		static const wchar_t* shader_cache_dir;
		static void AppendCapsDefines(const ShaderCapsSet& caps_set, std::vector<ShaderProgramInfo::Macro>& out_defines);

	public:
		ShaderCache();
		~ShaderCache();
		ShaderProgram* GetShaderProgram(const ShaderProgramInfo& info);
		ShaderProgram* GetShaderProgram(uint32_t vertex_hash, uint32_t fragment_hash, uint32_t compute_hash = 0);
	private:
		static uint32_t GetCombinedHash(uint32_t name_hash, const ShaderCapsSet& caps_set);
		static uint32_t GetDefinesHash(std::vector<ShaderProgramInfo::Macro> defines);
		ShaderModule* GetShaderModule(uint32_t hash);

	private:
		std::unordered_map<uint32_t, std::unique_ptr<ShaderModule>> module_cache;
		std::unordered_map<uint32_t, std::unique_ptr<ShaderProgram>> program_cache;
			

	};

}