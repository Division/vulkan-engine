#include "ReflectionInfo.h"
#include "ShaderDefines.h"
#include "ShaderResource.h"

namespace core { namespace Device {

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
			data.shader_texture = SHADER_SAMPLER_NAMES.at(data.name);
			samplers.push_back(data);
		}

		for (auto& storage_buffer : resources.storage_buffers)
		{
			StorageBufferData data;
			data.id = storage_buffer.id;
			data.name = storage_buffer.name;
			data.set = compiler.get_decoration(storage_buffer.id, spv::DecorationDescriptorSet);
			data.binding = compiler.get_decoration(storage_buffer.id, spv::DecorationBinding);
			data.ssbo_name = SHADER_BUFFER_NAMES.at(storage_buffer.name);

			storage_buffers.push_back(data);
		}
	}

} }