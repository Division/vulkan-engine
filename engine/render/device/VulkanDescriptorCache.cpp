#include "VulkanDescriptorCache.h"
#include "VulkanRenderState.h"
#include "render/shader/ShaderResource.h"
#include "render/shader/ShaderBindings.h"
#include "render/texture/Texture.h"
#include "Engine.h"
#include "utils/Math.h"

namespace core { namespace Device {

	VulkanDescriptorCache::VulkanDescriptorCache(vk::Device device)
	{
		const unsigned max_count = 10000;
		const unsigned SamplerSlotCount = 20;

		std::array<vk::DescriptorPoolSize, 7> pool_sizes = {
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, (int)ShaderBufferName::Count * max_count),
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformTexelBuffer, (int)ShaderTextureName::Count * max_count),
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, SamplerSlotCount * max_count),
			vk::DescriptorPoolSize(vk::DescriptorType::eSampler, SamplerSlotCount * max_count),
			vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, (int)ShaderTextureName::Count * max_count),
			vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, (int)ShaderTextureName::Count * max_count),
			vk::DescriptorPoolSize(vk::DescriptorType::eStorageBufferDynamic, (int)ShaderBufferName::Count * max_count)
		};

		const auto descriptor_pool_info = vk::DescriptorPoolCreateInfo(
			vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, max_count, (uint32_t)pool_sizes.size(), pool_sizes.data()
		);

		descriptor_pool = device.createDescriptorPoolUnique(descriptor_pool_info);
	}

	vk::DescriptorSet VulkanDescriptorCache::GetDescriptorSet(const ShaderBindings& bindings, const ShaderProgram::DescriptorSet& descriptor_set)
	{
		static thread_local DescriptorSetData set_data;

		memset(set_data.texture_bindings.data(), 0, sizeof(set_data.texture_bindings));
		memset(set_data.buffer_bindings.data(), 0, sizeof(set_data.buffer_bindings));

		assert(descriptor_set.bindings.size() == bindings.GetBufferBindings().size() + bindings.GetTextureBindings().size());

		// Getting hash for descriptor sets
		for (auto& binding : bindings.GetTextureBindings())
		{
			set_data.texture_bindings[binding.index] = vk::DescriptorImageInfo(vk::Sampler(), binding.texture->GetImageView());
		}

		for (auto& binding : bindings.GetBufferBindings())
		{
			set_data.buffer_bindings[binding.index].buffer = binding.buffer;
			set_data.buffer_bindings[binding.index].offset = 0;
			set_data.buffer_bindings[binding.index].range = binding.size;
		}

		uint32_t hashes[] =
		{
			FastHash(set_data.texture_bindings.data(), sizeof(set_data.texture_bindings)),
			FastHash(set_data.buffer_bindings.data(), sizeof(set_data.buffer_bindings)),
			FastHash(&descriptor_set.layout.get(), sizeof(descriptor_set.layout.get())) // include layout into the hash
		};

		uint32_t hash = FastHash(hashes, sizeof(hashes));
		
		{
			std::lock_guard<std::mutex> lock(mutex);
			auto iter = set_map.find(hash);
			if (iter != set_map.end())
				return iter->second;
		}

		vk::DescriptorSet result = CreateDescriptorSet(set_data, hash, descriptor_set);
		set_map.insert(std::make_pair(hash, result));
		return result;
	}

	vk::DescriptorSet VulkanDescriptorCache::CreateDescriptorSet(DescriptorSetData& data, uint32_t hash, const ShaderProgram::DescriptorSet& descriptor_set)
	{
		auto& layout = descriptor_set.layout.get();

		vk::DescriptorSet result;
		vk::DescriptorSetAllocateInfo alloc_info(descriptor_pool.get(), 1, &layout);

		auto& device = Engine::GetVulkanDevice();
		device.allocateDescriptorSets(&alloc_info, &result);
		// todo: handle failure

		utils::SmallVector<vk::WriteDescriptorSet, 15> writes;

		for (int i = 0; i < descriptor_set.bindings.size(); i++)
		{
			auto& binding = descriptor_set.bindings[i];

			auto binding_index = binding.address.binding;

			switch (binding.type)
			{
			case ShaderProgram::BindingType::Sampler:
			{
				assert(data.texture_bindings[binding_index].imageView);
				data.texture_bindings[binding_index].imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
				data.texture_bindings[binding_index].sampler = GetSampler(SamplerMode());

				writes.push_back(vk::WriteDescriptorSet(
					result,
					binding_index, 0, 1, vk::DescriptorType::eCombinedImageSampler,
					&data.texture_bindings[binding_index]
				));
				break;
			}

			case ShaderProgram::BindingType::UniformBuffer:
			case ShaderProgram::BindingType::StorageBuffer:
			{
				assert(data.buffer_bindings[binding_index].buffer);

				vk::DescriptorType buffer_descriptor_type = binding.type == ShaderProgram::BindingType::UniformBuffer 
					? vk::DescriptorType::eUniformBufferDynamic 
					: vk::DescriptorType::eStorageBufferDynamic;

				auto& binding_data = data.buffer_bindings[binding_index];
				
				writes.push_back(vk::WriteDescriptorSet(
					result,
					binding_index, 0, 1, buffer_descriptor_type,
					nullptr, &binding_data
				));
				break;
			}

			default:
				throw std::runtime_error("unsupported binding type");
			}
		}

		device.updateDescriptorSets(writes.size(), writes.data(), 0, nullptr);

		{
			std::lock_guard<std::mutex> lock(mutex);
			set_map.insert(std::make_pair(hash, result));
		}

		return result;
	}

	vk::Sampler VulkanDescriptorCache::GetSampler(const SamplerMode& sampler_mode)
	{
		auto hash = sampler_mode.GetHash();
		
		{
			std::lock_guard<std::mutex> lock(mutex_sampler);
			auto iter = sampler_cache.find(hash);
			if (iter != sampler_cache.end())
				return iter->second.get();
		}

		// TODO: proper sampler creation
		vk::SamplerCreateInfo sampler_info(
			{}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eNearest,
			vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
			0.0f, true, 16, false);

		auto device = Engine::GetVulkanDevice();
		auto sampler = device.createSamplerUnique(sampler_info);
		auto result = sampler.get();
		
		{
			std::lock_guard<std::mutex> lock(mutex_sampler);
			sampler_cache[hash] = std::move(sampler);
		}

		return result;
	}

} }