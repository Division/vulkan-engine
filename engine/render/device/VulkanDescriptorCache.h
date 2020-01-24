#pragma once

#include <mutex>
#include "CommonIncludes.h"
#include "VulkanCaps.h"
#include "render/shader/Shader.h"

namespace core { namespace Device {
	
	class ShaderBindings;
	class ShaderCache;
	class SamplerMode;

	class VulkanDescriptorCache
	{
	public:
		VulkanDescriptorCache(vk::Device device);

		vk::DescriptorSet GetDescriptorSet(const ShaderBindings& bindings, const ShaderProgram::DescriptorSet& descriptor_set);
		vk::Sampler GetSampler(const SamplerMode& sampler_mode);

	private:
		struct DescriptorSetData
		{
			std::array<vk::DescriptorImageInfo, caps::max_texture_bindings> texture_bindings;
			std::array<vk::DescriptorBufferInfo, caps::max_ubo_bindings> buffer_bindings;
		};

		vk::DescriptorSet CreateDescriptorSet(DescriptorSetData& data, uint32_t hash, const ShaderProgram::DescriptorSet& descriptor_set);

	private:
		ShaderCache* shader_cache;
		std::unordered_map<uint32_t, vk::DescriptorSet> set_map;
		std::unordered_map<uint32_t, vk::UniqueSampler> sampler_cache;
		vk::UniqueDescriptorPool descriptor_pool;
		std::mutex mutex;
		std::mutex mutex_sampler;
	};

} }