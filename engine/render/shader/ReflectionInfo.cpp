#include "ReflectionInfo.h"
#include "ShaderDefines.h"
#include "ShaderResource.h"
#include "utils/StringUtils.h"
#include "Shader.h"
#include <optional>

namespace Device {

	std::string ConvertHLSLName(const std::string& str)
	{
		if (str.find("type_") == 0)
			return str.substr(5);

		if (str.find("in_var_") == 0)
		{
			std::string result = str.substr(7);
			::utils::Lowercase(result);
			return result;
		}

		return str;
	}

	ReflectionInfo::UniformBufferData ReflectionInfo::GetUniformBufferData(spirv_cross::Resource& ubo, spirv_cross::CompilerGLSL& compiler)
	{
		UniformBufferData data;
		data.id = ubo.id;
		data.name = ConvertHLSLName(ubo.name);
		data.name_hash = ShaderProgram::GetParameterNameHash(data.name);
		data.set = compiler.get_decoration(ubo.id, spv::DecorationDescriptorSet);
		data.binding = compiler.get_decoration(ubo.id, spv::DecorationBinding);
		auto it = SHADER_BUFFER_NAMES.find(ConvertHLSLName(ubo.name));
		data.shader_buffer = it == SHADER_BUFFER_NAMES.end() ? ShaderBufferName::Default : it->second;

		const auto& type = compiler.get_type(ubo.base_type_id);
		data.size = (uint32_t)compiler.get_declared_struct_size(type);

		unsigned member_count = type.member_types.size();
		for (unsigned i = 0; i < member_count; i++)
		{
			BufferMember member;

			member.name = compiler.get_member_name(type.self, i);
			member.size = compiler.get_declared_struct_member_size(type, i);
			member.offset = compiler.type_struct_member_offset(type, i);
			member.name_hash = ShaderProgram::GetParameterNameHash(member.name);

			data.members.emplace_back(std::move(member));
		}

		return data;
	}

	ReflectionInfo::StorageBufferData ReflectionInfo::GetStorageBufferData(spirv_cross::Resource& ssbo, spirv_cross::CompilerGLSL& compiler)
	{
		StorageBufferData data;
		data.id = ssbo.id;
		data.name = ConvertHLSLName(ssbo.name);
		data.name_hash = ShaderProgram::GetParameterNameHash(data.name);
		data.set = compiler.get_decoration(ssbo.id, spv::DecorationDescriptorSet);
		data.binding = compiler.get_decoration(ssbo.id, spv::DecorationBinding);
		auto it = SHADER_BUFFER_NAMES.find(ConvertHLSLName(ssbo.name));
		data.storage_buffer_name = it == SHADER_BUFFER_NAMES.end() ? ShaderBufferName::DefaultStorage : it->second;

		const auto& type = compiler.get_type(ssbo.base_type_id);
		data.size = (uint32_t)compiler.get_declared_struct_size(type);
		
		// Can't know size for sure, go with maximum
		if (data.size == 0)
			data.size = 65535;

		unsigned member_count = type.member_types.size();
		for (unsigned i = 0; i < member_count; i++)
		{
			BufferMember member;

			member.name = compiler.get_member_name(type.self, i);
			if (member.name == "")
			{
				assert(member_count == 1); // Unnamed member, assuming default member name to be the buffer name
				member.name = data.name;
			}

			member.size = compiler.get_declared_struct_member_size(type, i);
			if (member.size == 0)
				member.size = 65535;

			member.offset = compiler.type_struct_member_offset(type, i);
			member.name_hash = ShaderProgram::GetParameterNameHash(member.name);

			data.members.emplace_back(std::move(member));
		}

		return data;
	}

	ReflectionInfo::ReflectionInfo(uint32_t* spirv_data, size_t count)
		: compiler(spirv_data, count)
	{
		auto resources = compiler.get_shader_resources();
		for (auto& entry : compiler.get_entry_points_and_stages())
		{
			entry_points.push_back({entry.name});
		}

		for (auto& sampler : resources.separate_samplers)
		{
			SamplerData data;
			data.id = sampler.id;
			data.name = sampler.name;
			data.set = compiler.get_decoration(sampler.id, spv::DecorationDescriptorSet);
			data.binding = compiler.get_decoration(sampler.id, spv::DecorationBinding);
			data.sampler_name = SHADER_SAMPLER_NAMES.at(sampler.name);

			samplers.push_back(data);
		}

		for (auto& ubo : resources.uniform_buffers)
		{
			uniform_buffers.push_back(GetUniformBufferData(ubo, compiler));
		}

		for (auto& input : resources.stage_inputs)
		{
			VertexAttribData data;
			data.id = input.id;
			data.name = input.name;
			data.set = compiler.get_decoration(input.id, spv::DecorationDescriptorSet);
			data.location = compiler.get_decoration(input.id, spv::DecorationLocation);
			auto attrib_iterator = VERTEX_ATTRIB_NAMES.find(ConvertHLSLName(data.name));
			data.vertex_attrib = attrib_iterator != VERTEX_ATTRIB_NAMES.end() ? attrib_iterator->second : VertexAttrib::Unknown; // unknown may be varyings (fragment input from vertex)
			vertex_attribs.push_back(data);
		}

		for (auto& sampler : resources.sampled_images)
		{
			CombinedImageSamplerData data;
			data.id = sampler.id;
			data.name = sampler.name;
			data.set = compiler.get_decoration(sampler.id, spv::DecorationDescriptorSet);
			data.binding = compiler.get_decoration(sampler.id, spv::DecorationBinding);
			auto iter = SHADER_TEXTURE_NAMES.find(ConvertHLSLName(data.name));
			if (iter != SHADER_TEXTURE_NAMES.end())
				data.shader_texture = iter->second;

			combined_image_samplers.push_back(data);
		}

		for (auto& sampler : resources.separate_images)
		{
			SeparateImageData data;
			data.id = sampler.id;
			data.name = sampler.name;
			data.set = compiler.get_decoration(sampler.id, spv::DecorationDescriptorSet);
			data.binding = compiler.get_decoration(sampler.id, spv::DecorationBinding);
			auto iter = SHADER_TEXTURE_NAMES.find(ConvertHLSLName(data.name));
			if (iter != SHADER_TEXTURE_NAMES.end())
				data.shader_texture = iter->second;

			separate_images.push_back(data);
		}

		for (auto& storage_buffer : resources.storage_buffers)
		{
			storage_buffers.push_back(GetStorageBufferData(storage_buffer, compiler));
		}

		for (auto& push_constant_buffer : resources.push_constant_buffers)
		{
			PushConstantsData data;
			data.id = push_constant_buffer.id;
			data.name = push_constant_buffer.name;
			data.size = 0;
			auto ranges = compiler.get_active_buffer_ranges(data.id);
			for (auto &range : ranges)
			{
				data.size += range.range;
			}
			
			if (ranges.size())
			{
				data.offset = ranges[0].offset;
				push_constants.push_back(data);
			}

		}
	}

}