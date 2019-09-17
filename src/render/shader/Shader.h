#pragma once

#include "render/device/VulkanContext.h"
#include "ReflectionInfo.h"

namespace core { namespace Device {
	
	class ReflectionInfo;

	class ShaderModule
	{
	public:
		ShaderModule() = default;
		ShaderModule(void* data, size_t size);
		ShaderModule(ShaderModule&&) = default;
		ShaderModule& operator= (ShaderModule&&) = default;

		vk::ShaderModule GetModule() const { return shader_module.get(); }
		const ReflectionInfo* GetReflectionInfo() const { return reflection_info.get(); }
		bool HasModule() const { return (VkShaderModule)shader_module.get() != VK_NULL_HANDLE; }
	private:
		vk::UniqueShaderModule shader_module;
		std::unique_ptr<ReflectionInfo> reflection_info;
	};


	class ShaderProgram
	{
	public:
		enum class Stage : int
		{
			Vertex = 0, Fragment, Count
		};

		void AddModule(ShaderModule shader_module, Stage stage);
		void Prepare();

		const ShaderModule& VertexModule() const { return vertex_module; }
		const ShaderModule& FragmentModule() const { return fragment_module; }
		vk::DescriptorSetLayout GetDescriptorSetLayout() const { return descriptor_set_layout.get(); }

	private:
		ShaderModule vertex_module;
		ShaderModule fragment_module;
	
		vk::UniqueDescriptorSetLayout descriptor_set_layout;
	};

} }