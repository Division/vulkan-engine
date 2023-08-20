#include "VulkanDescriptorCache.h"
#include "VulkanRenderState.h"
#include "render/shader/ShaderResource.h"
#include "render/shader/ShaderBindings.h"
#include "render/texture/Texture.h"
#include "Engine.h"
#include "utils/Math.h"

namespace Device {

	namespace
	{
		uint32_t defaultFrameDynamicDescriptorPoolMaxSets = 1024;
		std::vector<vk::DescriptorPoolSize> defaultFrameDynamicDescriptorPoolSizes = {
			{vk::DescriptorType::eSampler, 64},
			{vk::DescriptorType::eCombinedImageSampler, 512},
			{vk::DescriptorType::eSampledImage, 512},
			{vk::DescriptorType::eStorageImage, 256},
			{vk::DescriptorType::eUniformTexelBuffer, 256},
			{vk::DescriptorType::eStorageTexelBuffer, 256},
			{vk::DescriptorType::eUniformBuffer, 1024},
			{vk::DescriptorType::eStorageBuffer, 512},
			{vk::DescriptorType::eUniformBufferDynamic, 128},
			{vk::DescriptorType::eStorageBufferDynamic, 128},
			{vk::DescriptorType::eInputAttachment, 64},
	#if 0 // Not using these:
			{ VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT,   0 },
			{ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 0 },
	#endif
		};
	}

	vk::DescriptorSet VulkanDescriptorPool::AllocateDescriptorSet(vk::DescriptorSetLayout layout)
	{
		vk::DescriptorSetAllocateInfo info = {};
		info.descriptorSetCount = 1;
		info.pSetLayouts = &layout;

        for (uint32_t iTry = 0; iTry < 2; iTry++)
        {
            if (!poolList.empty())
            {
				VkDescriptorSet set = {};
                info.descriptorPool = *poolList[current];
				VkDescriptorSetAllocateInfo& vkInfo = info;
                auto result = vkAllocateDescriptorSets(Engine::GetVulkanContext()->GetDevice(), &vkInfo, &set);
				if (result == VK_SUCCESS)
					return set;
				else if (result == VK_ERROR_OUT_OF_POOL_MEMORY)
					current++;
				else
					throw std::runtime_error("failed allocating descriptor");
            }

            if (iTry == 0)
            {
                if (current == poolList.size())
                {
                    vk::DescriptorPoolCreateInfo dpInfo = {};
                    dpInfo.maxSets = defaultFrameDynamicDescriptorPoolMaxSets;
                    dpInfo.pPoolSizes = defaultFrameDynamicDescriptorPoolSizes.data();
                    dpInfo.poolSizeCount = (uint32_t)defaultFrameDynamicDescriptorPoolSizes.size();

					auto pool = Engine::GetVulkanContext()->GetDevice().createDescriptorPoolUnique(dpInfo);
                    poolList.push_back(std::move(pool));
                }
            }
        }

		throw std::runtime_error("failed creating frame descriptor set");
	}

	void VulkanDescriptorPool::Reset()
	{
		current = 0;
		for (auto& pool : poolList)
		{
			Engine::GetVulkanContext()->GetDevice().resetDescriptorPool(*pool, {});
		}
	}

	VulkanDescriptorCache::VulkanDescriptorCache(vk::Device device)
	{
		const unsigned max_count = 10000;
		const unsigned SamplerSlotCount = 20;

		std::array<vk::DescriptorPoolSize, 8> pool_sizes = {
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, (int)ShaderBufferName::Count * max_count),
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, (int)ShaderBufferName::Count * max_count),
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformTexelBuffer, (int)ShaderTextureName::Count * max_count),
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, SamplerSlotCount * max_count),
			vk::DescriptorPoolSize(vk::DescriptorType::eSampler, SamplerSlotCount * max_count),
			vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, (int)ShaderTextureName::Count * max_count),
			vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, (int)ShaderTextureName::Count * max_count),
			vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, (int)ShaderBufferName::Count * max_count)
		};

		const auto descriptor_pool_info = vk::DescriptorPoolCreateInfo(
			vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, max_count, (uint32_t)pool_sizes.size(), pool_sizes.data()
		);

		descriptor_pool = device.createDescriptorPoolUnique(descriptor_pool_info);
	}

	DescriptorSet VulkanDescriptorCache::GetDescriptorSet(const DescriptorSetBindings& bindings)
	{
		OPTICK_EVENT();

		static thread_local DescriptorSetData set_data;

		const auto& descriptor_set_layout = bindings.GetDescriptorSetLayout();

		assert(descriptor_set_layout.layout);

		PopulateDescriptorSetData(set_data, bindings);

		uint32_t hashes[] =
		{
			FastHash(set_data.texture_bindings.data(), sizeof(set_data.texture_bindings)),
			FastHash(set_data.buffer_bindings.data(), sizeof(set_data.buffer_bindings)),
			FastHash(&descriptor_set_layout.layout.get(), sizeof(descriptor_set_layout.layout.get())) // include layout into the hash
		};

		uint32_t hash = FastHash(hashes, sizeof(hashes));
		
		{
			std::lock_guard<std::mutex> lock(mutex);
			auto iter = set_map.find(hash);
			if (iter != set_map.end())
				return *iter->second;
		}

		auto& layout = descriptor_set_layout.layout.get();
		vk::DescriptorSet vk_descriptor_set;
		vk::DescriptorSetAllocateInfo alloc_info(descriptor_pool.get(), 1, &layout);

		auto& device = Engine::GetVulkanDevice();
		device.allocateDescriptorSets(&alloc_info, &vk_descriptor_set);
		UpdateDescriptorSet(vk_descriptor_set, set_data, descriptor_set_layout);
		{
			std::lock_guard<std::mutex> lock(mutex);
			auto it = set_map.insert(std::make_pair(hash, std::make_unique<DescriptorSet>(&descriptor_set_layout, vk_descriptor_set)));
			return *it.first->second;
		}
	}


	void VulkanDescriptorCache::PopulateDescriptorSetData(DescriptorSetData& data, const DescriptorSetBindings& bindings)
	{
		memset(data.texture_bindings.data(), 0, sizeof(data.texture_bindings));
		memset(data.buffer_bindings.data(), 0, sizeof(data.buffer_bindings));

		//assert(descriptor_set.bindings.size() == bindings.GetBufferBindings().size() + bindings.GetTextureBindings().size());

		// Getting hash for descriptor sets
		for (auto& binding : bindings.GetTextureBindings())
		{
			data.texture_bindings[binding.index] = vk::DescriptorImageInfo(vk::Sampler(), binding.image_view);
		}

		for (auto& binding : bindings.GetBufferBindings())
		{
			data.buffer_bindings[binding.index].buffer = binding.buffer;
			data.buffer_bindings[binding.index].offset = binding.offset;
			data.buffer_bindings[binding.index].range = binding.size;
		}
	}


	void VulkanDescriptorCache::UpdateDescriptorSet(vk::DescriptorSet descriptor_set, DescriptorSetData& data, const ShaderProgram::DescriptorSetLayout& layout)
	{
		utils::SmallVector<vk::WriteDescriptorSet, 15> writes;

		for (int i = 0; i < layout.bindings.size(); i++)
		{
			auto& binding = layout.bindings[i];

			auto binding_index = binding.address.binding;

			switch (binding.type)
			{
			case ShaderProgram::BindingType::CombinedImageSampler:
			case ShaderProgram::BindingType::SampledImage:
			case ShaderProgram::BindingType::Sampler:
			{
				assert(binding.type == ShaderProgram::BindingType::Sampler || data.texture_bindings[binding_index].imageView);
				data.texture_bindings[binding_index].imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
				bool is_combined = binding.type == ShaderProgram::BindingType::CombinedImageSampler;
				bool is_sampler = binding.type == ShaderProgram::BindingType::Sampler;
				ShaderSamplerName sampler_name = is_sampler ? (ShaderSamplerName)binding.id : ShaderSamplerName::LinearWrap;

				if (is_combined || is_sampler)
					data.texture_bindings[binding_index].sampler = GetSampler({sampler_name});

				vk::DescriptorType type = vk::DescriptorType::eSampledImage;
				if (is_combined)
					type = vk::DescriptorType::eCombinedImageSampler;
				else if (is_sampler)
					type = vk::DescriptorType::eSampler;

				writes.push_back(vk::WriteDescriptorSet(
					descriptor_set,
					binding_index, 0, 1, type,
					&data.texture_bindings[binding_index]
				));
				break;
			}

			case ShaderProgram::BindingType::UniformBuffer:
			case ShaderProgram::BindingType::UniformBufferDynamic:
			case ShaderProgram::BindingType::StorageBuffer:
			case ShaderProgram::BindingType::StorageBufferDynamic:
			{
				assert(data.buffer_bindings[binding_index].buffer);
				
				vk::DescriptorType buffer_descriptor_type;
				if (binding.type == ShaderProgram::BindingType::UniformBuffer)
					buffer_descriptor_type = vk::DescriptorType::eUniformBuffer;
				else if (binding.type == ShaderProgram::BindingType::UniformBufferDynamic)
					buffer_descriptor_type = vk::DescriptorType::eUniformBufferDynamic;
				else if (binding.type == ShaderProgram::BindingType::StorageBuffer)
					buffer_descriptor_type = vk::DescriptorType::eStorageBuffer;
				else if (binding.type == ShaderProgram::BindingType::StorageBufferDynamic)
					buffer_descriptor_type = vk::DescriptorType::eStorageBufferDynamic;
				else
					assert(false);

				auto& binding_data = data.buffer_bindings[binding_index];
				
				writes.push_back(vk::WriteDescriptorSet(
					descriptor_set,
					binding_index, 0, 1, buffer_descriptor_type,
					nullptr, &binding_data
				));
				break;
			}

			default:
				throw std::runtime_error("unsupported binding type");
			}
		}

		Engine::GetVulkanContext()->GetDevice().updateDescriptorSets(writes.size(), writes.data(), 0, nullptr);
	}


	DescriptorSet VulkanDescriptorCache::GetFrameDescriptorSet(const DescriptorSetBindings& bindings)
	{
		static thread_local DescriptorSetData set_data;
		PopulateDescriptorSetData(set_data, bindings);

		const auto& descriptor_set_layout = bindings.GetDescriptorSetLayout();
		vk::DescriptorSet descriptor_set = frame_descriptors[Engine::GetVulkanContext()->GetCurrentFrameRollOver()].AllocateDescriptorSet(*descriptor_set_layout.layout);

		UpdateDescriptorSet(descriptor_set, set_data, descriptor_set_layout);
		return DescriptorSet(&descriptor_set_layout, descriptor_set);
	}


	vk::Sampler VulkanDescriptorCache::GetSampler(const SamplerMode& sampler_mode)
	{
		auto hash = sampler_mode.GetHash();
		
		{
			std::scoped_lock lock(mutex_sampler);
			auto iter = sampler_cache.find(hash);
			if (iter != sampler_cache.end())
				return iter->second.get();
		}

		vk::SamplerCreateInfo sampler_info;

		switch (sampler_mode.name)
		{
		case ShaderSamplerName::PointWrap:
			sampler_info = vk::SamplerCreateInfo(
				{}, vk::Filter::eNearest, vk::Filter::eNearest, vk::SamplerMipmapMode::eNearest,
				vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
				0.0f, false, 0, false, vk::CompareOp::eNever, 0.0f, VK_LOD_CLAMP_NONE);
			break;
		case ShaderSamplerName::PointClamp:
			sampler_info = vk::SamplerCreateInfo(
				{}, vk::Filter::eNearest, vk::Filter::eNearest, vk::SamplerMipmapMode::eNearest,
				vk::SamplerAddressMode::eClampToEdge, vk::SamplerAddressMode::eClampToEdge, vk::SamplerAddressMode::eClampToEdge,
				0.0f, false, 0, false, vk::CompareOp::eNever, 0.0f, VK_LOD_CLAMP_NONE);
			break;
		case ShaderSamplerName::LinearWrap:
			sampler_info = vk::SamplerCreateInfo(
				{}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear,
				vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
				0.0f, true, 8, false, vk::CompareOp::eNever, 0.0f, VK_LOD_CLAMP_NONE);
			break;
		case ShaderSamplerName::LinearClamp:
			sampler_info = vk::SamplerCreateInfo(
				{}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear,
				vk::SamplerAddressMode::eClampToEdge, vk::SamplerAddressMode::eClampToEdge, vk::SamplerAddressMode::eClampToEdge,
				0.0f, true, 8, false, vk::CompareOp::eNever, 0.0f, VK_LOD_CLAMP_NONE);
			break;
		default:
			throw std::runtime_error("unsupported sampler");
		}

		auto device = Engine::GetVulkanDevice();
		auto sampler = device.createSamplerUnique(sampler_info);
		auto result = sampler.get();
		
		{
			std::scoped_lock lock(mutex_sampler);
			sampler_cache[hash] = std::move(sampler);
		}

		return result;
	}
	
	void VulkanDescriptorCache::ResetFrameDescriptors()
	{
		frame_descriptors[Engine::GetVulkanContext()->GetCurrentFrameRollOver()].Reset();
	}

}