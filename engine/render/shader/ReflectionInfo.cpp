#include "ReflectionInfo.h"
#include "ShaderDefines.h"
#include "ShaderResource.h"

namespace Device {

	ReflectionInfo::ReflectionInfo(uint32_t* spirv_data, size_t count)
		: compiler(spirv_data, count)
	{
		auto resources = compiler.get_shader_resources();

		for (auto& ubo : resources.uniform_buffers)
		{
			UniformBufferData data;
			data.id = ubo.id;
			data.name = ubo.name;
			data.set = compiler.get_decoration(ubo.id, spv::DecorationDescriptorSet);
			data.binding = compiler.get_decoration(ubo.id, spv::DecorationBinding);
			data.shader_buffer = SHADER_BUFFER_NAMES.at(ubo.name); // TODO: support for Unknown arbitrary buffer / sampler names

			uniform_buffers.push_back(data);
		}

		for (auto& input : resources.stage_inputs)
		{
			VertexAttribData data;
			data.id = input.id;
			data.name = input.name;
			data.set = compiler.get_decoration(input.id, spv::DecorationDescriptorSet);
			data.location = compiler.get_decoration(input.id, spv::DecorationLocation);
			auto attrib_iterator = VERTEX_ATTRIB_NAMES.find(data.name);
			data.vertex_attrib = attrib_iterator != VERTEX_ATTRIB_NAMES.end() ? attrib_iterator->second : VertexAttrib::Unknown; // unknown may be varyings (fragment input from vertex)
			vertex_attribs.push_back(data);
		}

		for (auto& sampler : resources.sampled_images)
		{
			SamplerData data;
			data.id = sampler.id;
			data.name = sampler.name;
			data.set = compiler.get_decoration(sampler.id, spv::DecorationDescriptorSet);
			data.binding = compiler.get_decoration(sampler.id, spv::DecorationBinding);
			auto iter = SHADER_SAMPLER_NAMES.find(data.name);
			if (iter != SHADER_SAMPLER_NAMES.end())
				data.shader_texture = iter->second;

			samplers.push_back(data);
		}

		for (auto& storage_buffer : resources.storage_buffers)
		{
			StorageBufferData data;
			data.id = storage_buffer.id;
			data.name = storage_buffer.name;
			data.set = compiler.get_decoration(storage_buffer.id, spv::DecorationDescriptorSet);
			data.binding = compiler.get_decoration(storage_buffer.id, spv::DecorationBinding);
			auto iter = SHADER_BUFFER_NAMES.find(storage_buffer.name);
			if (iter != SHADER_BUFFER_NAMES.end())
				data.storage_buffer_name = iter->second;

			storage_buffers.push_back(data);
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