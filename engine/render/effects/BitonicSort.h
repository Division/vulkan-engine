#pragma once

#include <memory>

namespace Device
{
	class Texture;
	class ShaderCache;
	class ResourceBindings;
	class ConstantBindings;
	class ShaderProgram;
	class GPUBuffer;
	class VulkanBuffer;
	class VulkanRenderState;
}

namespace render
{
	class BitonicSort
	{
	public:
		BitonicSort(Device::ShaderCache& shader_cache);
		~BitonicSort();
		void Process(
			Device::VulkanRenderState& state,
			const Device::VulkanBuffer& key_index_list,
			const Device::VulkanBuffer& counter_buffer,
			uint32_t counter_offset,
			bool partially_presorted,
			bool ascending
		);

	private:
		std::unique_ptr<Device::GPUBuffer> dispatch_args;
		Device::ShaderProgram* pre_sort_shader;
		Device::ShaderProgram* inner_sort_shader;
		Device::ShaderProgram* outer_sort_shader;
		Device::ShaderProgram* indirect_args_shader;
	};
}