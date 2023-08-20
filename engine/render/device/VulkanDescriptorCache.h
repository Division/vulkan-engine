#pragma once

#include <mutex>
#include "CommonIncludes.h"
#include "VulkanCaps.h"
#include "render/shader/Shader.h"
#include <unordered_map>
#include <memory>
#include "VulkanRenderState.h"

namespace Device {
	
	class DescriptorSetBindings;
	class ShaderCache;
	class SamplerMode;

	class VulkanDescriptorPool
	{
	public:
		vk::DescriptorSet AllocateDescriptorSet(vk::DescriptorSetLayout layout);
		void Reset();

	private:
		uint32_t current = 0;
		std::vector<vk::UniqueDescriptorPool> poolList;
	};

	class VulkanDescriptorCache
	{
	public:
		VulkanDescriptorCache(vk::Device device);

		DescriptorSet GetDescriptorSet(const DescriptorSetBindings& bindings);
		DescriptorSet GetFrameDescriptorSet(const DescriptorSetBindings& bindings);
		vk::Sampler GetSampler(const SamplerMode& sampler_mode); // TODO: better move out of this class

		void ResetFrameDescriptors();

	private:
		struct DescriptorSetData
		{
			std::array<vk::DescriptorImageInfo, caps::max_texture_bindings> texture_bindings;
			std::array<vk::DescriptorBufferInfo, caps::max_ubo_bindings> buffer_bindings;
		};

		void PopulateDescriptorSetData(DescriptorSetData& data, const DescriptorSetBindings& bindings);
		void UpdateDescriptorSet(vk::DescriptorSet descriptor_set, DescriptorSetData& data, const ShaderProgram::DescriptorSetLayout& layout);

	private:
		std::unordered_map<uint32_t, std::unique_ptr<DescriptorSet>> set_map;
		std::unordered_map<uint32_t, vk::UniqueSampler> sampler_cache;
		std::array<VulkanDescriptorPool, caps::MAX_FRAMES_IN_FLIGHT> frame_descriptors;
		vk::UniqueDescriptorPool descriptor_pool;
		std::mutex mutex;
		std::mutex mutex_sampler;
	};

}