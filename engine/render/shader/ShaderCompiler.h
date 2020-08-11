#pragma once

#include <string>
#include <vector>
#include "render/shader/ShaderCache.h"

namespace Device
{
	namespace ShaderCompiler
	{
		struct CompilationResult
		{
			std::vector<uint8_t> data;
			std::string error;
		};

		bool CompileShader(ShaderCache& shader_cache, const ShaderProgramInfo::ShaderData& shader_data, CompilationResult& out_result);
	}
}