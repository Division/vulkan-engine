#include "Shader.h"
#include "CommonIncludes.h"
#include "Engine.h"

namespace core { namespace Device {
	
	ShaderModule::ShaderModule(void* data, size_t size)
	{
		vk::ShaderModuleCreateInfo create_info(vk::ShaderModuleCreateFlags(), size, (uint32_t*)data);
		shader_module = Engine::GetVulkanContext()->GetDevice().createShaderModuleUnique(create_info);
		reflection = std::make_unique<spirv_cross::CompilerGLSL>((uint32_t*)data, size / 4);
		auto resources = reflection->get_shader_resources();
	}
	
	void ShaderProgram::AddModule(ShaderModule shader_module, Stage stage)
	{
		auto& module_var = stage == Stage::Vertex ? vertex_module : fragment_module;
		if (module_var.HasModule())
			throw std::runtime_error("Shader module already set");

		module_var = std::move(shader_module);
	}

} }