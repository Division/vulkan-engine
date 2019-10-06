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
		enum class Stage : unsigned
		{
			Vertex = 1 << 0,
			Fragment = 1 << 1
		};

		enum class BindingType : unsigned
		{
			Sampler = 0,
			UniformBuffer,
			Count
		};

		struct BindingAddress
		{
			unsigned set;
			unsigned binding;
		};

		struct BindingData
		{
			BindingType type;
			uint32_t id; // Engine internal identifier for texture/buffer
			unsigned stage_flags;
			std::string name;
			BindingAddress address;
		};

		struct DescriptorSet
		{
			unsigned set_index;
			std::vector<vk::DescriptorSetLayoutBinding> layout_bindings;
			vk::UniqueDescriptorSetLayout layout;
			std::vector<BindingData> bindings;
			bool Empty() const { return bindings.empty(); }
		};

		static const unsigned max_descriptor_sets = 4;

		ShaderProgram();
		void AddModule(ShaderModule shader_module, Stage stage);
		void Prepare();

		const ShaderModule& VertexModule() const { return vertex_module; }
		const ShaderModule& FragmentModule() const { return fragment_module; }
		
		const auto& GetDescriptorSets() const { return descriptor_sets; }
		const DescriptorSet* GetDescriptorSet(unsigned set) const { return &descriptor_sets[set]; };
		const BindingData* GetBinding(unsigned set, unsigned binding) const;
		const BindingData* GetBindingByName(const std::string& name) const;

		uint64_t GetHash() const;

	private:
		void AppendBindings(const ShaderModule& module, ShaderProgram::Stage stage, std::map<std::pair<unsigned, unsigned>, int>& existing_bindings);

	private:
		ShaderModule vertex_module;
		ShaderModule fragment_module;
		
		std::unordered_map<std::string, BindingAddress> name_binding_map;
		std::array<DescriptorSet, max_descriptor_sets> descriptor_sets;
	};

} }