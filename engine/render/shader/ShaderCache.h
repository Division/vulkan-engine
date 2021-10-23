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
			ShaderData& operator=(ShaderData&&) = default;
			ShaderData() = default;
			ShaderData(const ShaderData& other) = default;
			ShaderData(ShaderData&& other) = default;
			ShaderData(ShaderProgram::Stage stage, std::wstring path, std::string entry_point = "main", std::vector<Macro> defines = {});
		};
		
		const ShaderData* GetShaderData(ShaderProgram::Stage stage) const;

		ShaderProgramInfo& AddShader(ShaderProgram::Stage stage, std::wstring path, std::string entry_point = "main", std::vector<Macro> defines = {});
		ShaderProgramInfo& AddShader(ShaderData&& shader_data);
		
		std::vector<ShaderData> shaders;
		void Clear() { shaders.clear(); };
	};

	class ShaderCache
	{
	public:
		static uint32_t GetShaderProgramInfoHash(const ShaderProgramInfo& program_info);
		static uint32_t GetShaderDataHash(const ShaderProgramInfo::ShaderData& data);
		static std::wstring GetShaderCachePath(uint32_t shader_hash);
		static const wchar_t* shader_cache_dir;
		static void AppendCapsDefines(const ShaderCapsSet& caps_set, std::vector<ShaderProgramInfo::Macro>& out_defines);

	public:
		ShaderCache();
		~ShaderCache();

		static const std::vector<uint8_t>& GetShaderSource(const std::wstring& filename);
		ShaderProgram* GetShaderProgram(const ShaderProgramInfo& info);
	private:
		class PrepareShaderJob;
		static uint32_t GetShaderSourceHash(const std::wstring& path);
		static uint32_t GetCombinedHash(uint32_t name_hash, const ShaderCapsSet& caps_set);
		static uint32_t GetDefinesHash(std::vector<ShaderProgramInfo::Macro> defines);
		ShaderModule* GetShaderModule(const ShaderProgramInfo::ShaderData& shader_data);
		bool LoadShaderModule(ShaderModule& module, const ShaderProgramInfo::ShaderData& shader_data);

	private:
		inline static std::unordered_map<std::wstring, std::vector<uint8_t>> source_cache;
		inline static std::mutex source_mutex;
		std::mutex program_mutex;
		std::mutex module_mutex;
		std::unordered_map<uint32_t, std::unique_ptr<ShaderModule>> module_cache;
		std::unordered_map<uint32_t, std::unique_ptr<ShaderProgram>> program_cache;
	};

}