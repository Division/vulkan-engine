#pragma once

#include "CommonIncludes.h"
#include "SPIRV-Cross/spirv_glsl.hpp"
#include "render/mesh/VertexAttrib.h"
#include "ShaderResource.h"

namespace core { namespace Device {

	class ReflectionInfo
	{
	public:
		struct UniformBufferData
		{
			std::string name;
			unsigned set;
			unsigned binding;
		};

		struct VertexAttribData
		{
			std::string name;
			unsigned set;
			unsigned location;
			VertexAttrib vertex_attrib;
		};

		struct SamplerData
		{
			std::string name;
			unsigned binding;
			ShaderResourceName shader_resource;
		};

		ReflectionInfo(uint32_t* spirv_data, size_t count);

		const std::vector<UniformBufferData>& UniformBuffers() const { return uniform_buffers; }
		const std::vector<VertexAttribData>& VertexAttribs() const { return vertex_attribs; }
		const std::vector<SamplerData>& Samplers() const { return samplers; }

	private:
		spirv_cross::CompilerGLSL compiler;
		std::vector<UniformBufferData> uniform_buffers;
		std::vector<VertexAttribData> vertex_attribs;
		std::vector<SamplerData> samplers;

	};

} }