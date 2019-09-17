#include "ReflectionInfo.h"
#include "ShaderDefines.h"

namespace core { namespace Device {

	ReflectionInfo::ReflectionInfo(uint32_t* spirv_data, size_t count)
		: compiler(spirv_data, count)
	{
		auto resources = compiler.get_shader_resources();

		for (auto& ubo : resources.uniform_buffers)
		{
			UniformBufferData data;
			data.name = ubo.name;
			data.set = compiler.get_decoration(ubo.id, spv::DecorationDescriptorSet);
			data.binding = compiler.get_decoration(ubo.id, spv::DecorationBinding);
			uniform_buffers.push_back(data);
		}

		for (auto& input : resources.stage_inputs)
		{
			VertexAttribData data;
			data.name = input.name;
			data.set = compiler.get_decoration(input.id, spv::DecorationDescriptorSet);
			data.location = compiler.get_decoration(input.id, spv::DecorationLocation);
			auto attrib_iterator = VERTEX_ATTRIB_NAMES.find(data.name);
			data.vertex_attrib = attrib_iterator != VERTEX_ATTRIB_NAMES.end() ? attrib_iterator->second : VertexAttrib::Unknown; // unknown may be varyings (fragment vertex input)
			vertex_attribs.push_back(data);
		}

		for (auto& sampler : resources.sampled_images)
		{
			SamplerData data;
			data.name = sampler.name;
			data.binding = compiler.get_decoration(sampler.id, spv::DecorationBinding);
			data.shader_resource = SHADER_SAMPLER_NAMES.at(data.name);
			samplers.push_back(data);
		}
	}

} }