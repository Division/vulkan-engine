#include "Shader.h"
#include "CommonIncludes.h"
#include "Engine.h"

namespace core { namespace Device {
	
	ShaderModule::ShaderModule(void* data, size_t size)
	{
		vk::ShaderModuleCreateInfo create_info(vk::ShaderModuleCreateFlags(), size, (uint32_t*)data);
		shader_module = Engine::GetVulkanContext()->GetDevice().createShaderModuleUnique(create_info);
		reflection_info = std::make_unique<ReflectionInfo>((uint32_t*)data, size / sizeof(uint32_t));
	}
	
	void ShaderProgram::AddModule(ShaderModule shader_module, Stage stage)
	{
		auto& module_var = stage == Stage::Vertex ? vertex_module : fragment_module;
		if (module_var.HasModule())
			throw std::runtime_error("Shader module already set");

		module_var = std::move(shader_module);
	}

	std::map<ShaderProgram::Stage, vk::ShaderStageFlagBits> shader_stage_flag_map =
	{
		{ ShaderProgram::Stage::Vertex, vk::ShaderStageFlagBits::eVertex },
		{ ShaderProgram::Stage::Fragment, vk::ShaderStageFlagBits::eFragment },
	};

	void GetDescriptorSetLayoutBinding(const ShaderModule& module, ShaderProgram::Stage stage, std::vector<vk::DescriptorSetLayoutBinding>& out_result)
	{
		auto* reflection = module.GetReflectionInfo();
		auto stage_flags = shader_stage_flag_map.at(stage);
		
		for (auto& sampler : reflection->Samplers())
		{
			vk::DescriptorSetLayoutBinding binding(sampler.binding, vk::DescriptorType::eCombinedImageSampler, 1, shader_stage_flag_map.at(stage));
			out_result.push_back(binding);
		}

		for (auto& ubo : reflection->UniformBuffers())
		{
			vk::DescriptorSetLayoutBinding binding(ubo.binding, vk::DescriptorType::eUniformBuffer, 1, shader_stage_flag_map.at(stage));
			out_result.push_back(binding);
		}
	}

	void ShaderProgram::Prepare()
	{
		std::vector<vk::DescriptorSetLayoutBinding> bindings;

		if (vertex_module.HasModule())
			GetDescriptorSetLayoutBinding(vertex_module, Stage::Vertex, bindings);

		if (fragment_module.HasModule())
			GetDescriptorSetLayoutBinding(fragment_module, Stage::Fragment, bindings);

		auto device = Engine::GetVulkanDevice();
		vk::DescriptorSetLayoutCreateInfo layout_info({}, bindings.size(), bindings.data());

		descriptor_set_layout = device.createDescriptorSetLayoutUnique(layout_info);
	}

	uint64_t ShaderProgram::GetHash() const
	{
		return (uint64_t)this;
	}

} }