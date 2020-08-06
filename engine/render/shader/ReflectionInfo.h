#pragma once

#include "CommonIncludes.h"
#include "SPIRV-Cross/spirv_glsl.hpp"
#include "render/mesh/VertexAttrib.h"
#include "ShaderResource.h"

namespace Device {

	class ReflectionInfo
	{
	public:
		struct UniformBufferData
		{
			uint32_t id;
			std::string name;
			unsigned set;
			unsigned binding;
			ShaderBufferName shader_buffer;
		};

		struct VertexAttribData
		{
			uint32_t id;
			std::string name;
			unsigned set;
			unsigned location;
			VertexAttrib vertex_attrib;
		};

		struct SamplerData
		{
			uint32_t id;
			std::string name;
			unsigned set;
			unsigned binding;
			ShaderTextureName shader_texture = ShaderTextureName::Unknown;
		};

		struct StorageBufferData
		{
			uint32_t id;
			std::string name;
			unsigned set;
			unsigned binding;
			ShaderBufferName storage_buffer_name = ShaderBufferName::Unknown;
		};

		struct PushConstantsData
		{
			uint32_t id;
			uint32_t offset;
			uint32_t size;
			std::string name;
		};

		ReflectionInfo(uint32_t* spirv_data, size_t count);

		const std::vector<UniformBufferData>& UniformBuffers() const { return uniform_buffers; }
		const std::vector<StorageBufferData>& StorageBuffers() const { return storage_buffers; }
		const std::vector<VertexAttribData>& VertexAttribs() const { return vertex_attribs; }
		const std::vector<SamplerData>& Samplers() const { return samplers; }
		const std::vector<PushConstantsData>& PushConstants() const { return push_constants; }

	private:
		spirv_cross::CompilerGLSL compiler;
		std::vector<UniformBufferData> uniform_buffers;
		std::vector<VertexAttribData> vertex_attribs;
		std::vector<StorageBufferData> storage_buffers;
		std::vector<SamplerData> samplers;
		std::vector<PushConstantsData> push_constants;

	};

}