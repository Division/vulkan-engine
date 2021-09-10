#pragma once

#include <mutex>
#include "CommonIncludes.h"
#include "VulkanCaps.h"
#include "render/shader/Shader.h"
#include <unordered_map>
#include <memory>

namespace Device {
	
	class DescriptorSetBindings;
	class ShaderCache;
	class SamplerMode;
	class DescriptorSet;

	class VulkanDescriptorCache
	{
	public:
		VulkanDescriptorCache(vk::Device device);

		DescriptorSet* GetDescriptorSet(const DescriptorSetBindings& bindings);
		vk::Sampler GetSampler(const SamplerMode& sampler_mode); // TODO: better move out of this class

	private:
		struct DescriptorSetData
		{
			std::array<vk::DescriptorImageInfo, caps::max_texture_bindings> texture_bindings;
			std::array<vk::DescriptorBufferInfo, caps::max_ubo_bindings> buffer_bindings;
		};

		vk::DescriptorSet CreateDescriptorSet(DescriptorSetData& data, uint32_t hash, const ShaderProgram::DescriptorSetLayout& descriptor_set);

	private:
		std::unordered_map<uint32_t, std::unique_ptr<DescriptorSet>> set_map;
		std::unordered_map<uint32_t, vk::UniqueSampler> sampler_cache;
		vk::UniqueDescriptorPool descriptor_pool;
		std::mutex mutex;
		std::mutex mutex_sampler;
	};

}