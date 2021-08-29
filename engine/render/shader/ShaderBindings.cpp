#include "render/texture/Texture.h"
#include "render/buffer/VulkanBuffer.h"
#include "render/renderer/SceneRenderer.h"
#include "Engine.h"

#include "ShaderBindings.h"

namespace Device {

	void ResourceBindings::AddTextureBinding(const std::string& name, Texture* texture)
	{
		AddTextureBinding(ShaderProgram::GetParameterNameHash(name), texture);
	}

	void ResourceBindings::AddTextureBinding(uint32_t name_hash, Texture* texture)
	{
		auto index = GetTextureBindingIndex(name_hash);
		if (index == (uint32_t)-1)
		{
			texture_bindings.push_back({});
			index = texture_bindings.size() - 1;
		}

		texture_bindings[index].name_hash = name_hash;
		texture_bindings[index].texture = texture;
	}

	Texture* ResourceBindings::GetTextureBinding(uint32_t name_hash) const
	{
		auto index = GetTextureBindingIndex(name_hash);
		return index < texture_bindings.size() ? texture_bindings[index].texture : nullptr;
	}

	uint32_t ResourceBindings::GetTextureBindingIndex(uint32_t name_hash) const
	{
		auto it = std::find_if(texture_bindings.begin(), texture_bindings.end(), [name_hash](const TextureResourceBinding& binding) { return binding.name_hash == name_hash; });
		return it == texture_bindings.end() ? -1 : std::distance(texture_bindings.begin(), it);
	}

	void ResourceBindings::AddBufferBinding(const std::string& name, VulkanBuffer* buffer, uint32_t size, uint32_t dynamic_offset)
	{
		AddBufferBinding(ShaderProgram::GetParameterNameHash(name), buffer, size, dynamic_offset);
	}

	void ResourceBindings::AddBufferBinding(uint32_t name_hash, VulkanBuffer* buffer, uint32_t size, uint32_t dynamic_offset)
	{
		auto index = GetBufferBindingIndex(name_hash);
		if (index == (uint32_t)-1)
		{
			buffer_bindings.push_back({});
			index = buffer_bindings.size() - 1;
		}

		buffer_bindings[index].name_hash = name_hash;
		buffer_bindings[index].buffer = buffer;
		buffer_bindings[index].size = size;
		buffer_bindings[index].dynamic_offset = dynamic_offset;
	}

	std::optional<ResourceBindings::BufferResourceBinding> ResourceBindings::GetBufferBinding(uint32_t name_hash) const
	{
		auto index = GetBufferBindingIndex(name_hash);

		if (index >= buffer_bindings.size())
			return std::nullopt;
		
		return buffer_bindings[index];
	}

	uint32_t ResourceBindings::GetBufferBindingIndex(uint32_t name_hash) const
	{
		auto it = std::find_if(buffer_bindings.begin(), buffer_bindings.end(), [name_hash](const BufferResourceBinding& binding) { return binding.name_hash == name_hash; });
		return it == buffer_bindings.end() ? -1 : std::distance(buffer_bindings.begin(), it);
	}

	void ResourceBindings::Clear()
	{
		texture_bindings.clear();
		buffer_bindings.clear();
	}

	void ResourceBindings::Merge(const ResourceBindings& other)
	{
		for (auto& texture_binding : other.texture_bindings)
			AddTextureBinding(texture_binding.name_hash, texture_binding.texture);

		for (auto& buffer_binding : other.buffer_bindings)
			AddBufferBinding(buffer_binding.name_hash, buffer_binding.buffer, buffer_binding.size);
	}

	DescriptorSetBindings::DescriptorSetBindings(const Device::ResourceBindings& resource_bindings, const ShaderProgram::DescriptorSetLayout& descriptor_set_layout)
		: descriptor_set_layout(descriptor_set_layout)
	{
		SetupWithResourceBindings(resource_bindings, descriptor_set_layout);
	}

	DescriptorSetBindings::DescriptorSetBindings(const ShaderProgram::DescriptorSetLayout& descriptor_set_layout)
		: descriptor_set_layout(descriptor_set_layout)
	{
	}

	void DescriptorSetBindings::SetupWithResourceBindings(const Device::ResourceBindings& resource_bindings, const ShaderProgram::DescriptorSetLayout& descriptor_set_layout)
	{
		Clear();

		for (auto& binding : descriptor_set_layout.bindings)
		{
			auto& address = binding.address;
			switch (binding.type)
			{
			case ShaderProgram::BindingType::CombinedImageSampler:
			case ShaderProgram::BindingType::SampledImage:
			{
				auto* texture = resource_bindings.GetTextureBinding(binding.name_hash);
				if (!texture)
					texture = Engine::Get()->GetSceneRenderer()->GetBlankTexture();

				AddTextureBinding(address.binding, texture);
				break;
			}

			case ShaderProgram::BindingType::UniformBuffer:
			case ShaderProgram::BindingType::UniformBufferDynamic:
			case ShaderProgram::BindingType::StorageBuffer:
			{
				auto buffer_binding = resource_bindings.GetBufferBinding(binding.name_hash);
				if (!buffer_binding)
					throw std::runtime_error("Buffer required by shader is missing in the ResourceBindings");

				vk::Buffer buffer = buffer_binding->buffer->Buffer();
				const size_t size = buffer_binding->size;
				const uint32_t dynamic_offset = GetShaderBufferHasDynamicOffset(buffer_binding->name_hash) ? buffer_binding->dynamic_offset : -1;

				AddBufferBinding(address.binding, 0, size, buffer, dynamic_offset);
				break;
			}

			case ShaderProgram::BindingType::Sampler: break; // don't accumulate samplers

			default:
				throw std::runtime_error("unknown shader binding");
			}
		}

		UpdateBindings();
	}

	void DescriptorSetBindings::AddTextureBindingSafe(unsigned index, const Texture* texture)
	{
		if (index == -1)
			return;

		AddTextureBinding(index, texture);
	}

	void DescriptorSetBindings::AddBufferBindingSafe(unsigned index, size_t offset, size_t size, vk::Buffer buffer)
	{
		if (index == -1)
			return;

		AddBufferBinding(index, offset, size, buffer);
	}

	void DescriptorSetBindings::AddTextureBinding(unsigned index, const Texture* texture)
	{
		texture_bindings.push_back(TextureBinding{ (unsigned char)index, texture });
	}

	void DescriptorSetBindings::AddBufferBinding(unsigned index, size_t offset, size_t size, vk::Buffer buffer, size_t dynamic_offset)
	{
		buffer_bindings.push_back(BufferBinding{ (unsigned char)index, (unsigned)offset, (unsigned)dynamic_offset, (unsigned)size, buffer });
		UpdateBindings();
	}

	void DescriptorSetBindings::UpdateBindings()
	{
		std::sort(buffer_bindings.begin(), buffer_bindings.end());
		
		dynamic_offsets.clear();
		for (auto& binding : buffer_bindings)
			if (binding.dynamic_offset != -1)
				dynamic_offsets.push_back(binding.dynamic_offset);
	}

	int DescriptorSetBindings::GetBindingIndex(uint32_t index, ShaderProgram::BindingType type)
	{
		auto predicate = [index](auto& binding)
		{
			return binding.index == index;
		};

		switch (type)
		{
		case ShaderProgram::BindingType::CombinedImageSampler:
		case ShaderProgram::BindingType::SampledImage:
			{
				auto it = std::find_if(texture_bindings.begin(), texture_bindings.end(), predicate);
				return it != texture_bindings.end() ? it - texture_bindings.begin() : -1;
			}

		case ShaderProgram::BindingType::StorageBuffer:
		case ShaderProgram::BindingType::UniformBuffer:
		case ShaderProgram::BindingType::UniformBufferDynamic:
			{
				auto it = std::find_if(buffer_bindings.begin(), buffer_bindings.end(), predicate);
				return it != buffer_bindings.end() ? it - buffer_bindings.begin() : -1;
			}

		case ShaderProgram::BindingType::Sampler: 
			return -1;

		default:
			assert(false);
			return -1;
		}
	}

	void DescriptorSetBindings::Clear()
	{
		texture_bindings.clear();
		buffer_bindings.clear();
		dynamic_offsets.clear();
	}

}