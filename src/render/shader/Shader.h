#pragma once

#include "render/device/VulkanContext.h"
#include "SPIRV-Cross/spirv_glsl.hpp"

namespace core { namespace Device {
	
	class ShaderModule
	{
	public:
		ShaderModule() = default;
		ShaderModule(void* data, size_t size);
		ShaderModule(ShaderModule&&) = default;
		ShaderModule& operator= (ShaderModule&&) = default;

		vk::ShaderModule GetModule() const { return shader_module.get(); }
		bool HasModule() const { return (VkShaderModule)shader_module.get() != VK_NULL_HANDLE; }
	private:
		vk::UniqueShaderModule shader_module;
		std::unique_ptr<spirv_cross::CompilerGLSL> reflection;
	};


	class ShaderProgram
	{
	public:
		enum class Stage : int
		{
			Vertex, Fragment
		};

		void AddModule(ShaderModule shader_module, Stage stage);

	private:
		ShaderModule vertex_module;
		ShaderModule fragment_module;
		
	};

} }