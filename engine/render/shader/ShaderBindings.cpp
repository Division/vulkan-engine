#include "render/texture/Texture.h"
#include "render/buffer/VulkanBuffer.h"
#include "render/buffer/ConstantBuffer.h"
#include "render/renderer/SceneRenderer.h"
#include "render/renderer/SceneBuffers.h"
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

	void ResourceBindings::AddDynamicBufferBinding(const std::string& name, ConstantBuffer* buffer)
	{
		AddDynamicBufferBinding(ShaderProgram::GetParameterNameHash(name), buffer);
	}

	void ResourceBindings::AddDynamicBufferBinding(uint32_t name_hash, ConstantBuffer* buffer)
	{
		auto index = GetDynamicBufferBindingIndex(name_hash);
		if (index == (uint32_t)-1)
		{
			dynamic_buffer_bindings.push_back({});
			index = dynamic_buffer_bindings.size() - 1;
		}

		dynamic_buffer_bindings[index].name_hash = name_hash;
		dynamic_buffer_bindings[index].constant_buffer = buffer;
	}

	std::optional<ResourceBindings::DynamicBufferResourceBinding> ResourceBindings::GetDynamicBufferBinding(uint32_t name_hash) const
	{
		auto index = GetDynamicBufferBindingIndex(name_hash);

		if (index >= dynamic_buffer_bindings.size())
			return std::nullopt;

		return dynamic_buffer_bindings[index];
	}

	uint32_t ResourceBindings::GetDynamicBufferBindingIndex(uint32_t name_hash) const
	{
		auto it = std::find_if(dynamic_buffer_bindings.begin(), dynamic_buffer_bindings.end(), [name_hash](const DynamicBufferResourceBinding& binding) { return binding.name_hash == name_hash; });
		return it == dynamic_buffer_bindings.end() ? -1 : std::distance(dynamic_buffer_bindings.begin(), it);
	}

	void ResourceBindings::Clear()
	{
		texture_bindings.clear();
		buffer_bindings.clear();
		dynamic_buffer_bindings.clear();
	}

	void ResourceBindings::Merge(const ResourceBindings& other)
	{
		for (auto& texture_binding : other.texture_bindings)
			AddTextureBinding(texture_binding.name_hash, texture_binding.texture);

		for (auto& buffer_binding : other.buffer_bindings)
			AddBufferBinding(buffer_binding.name_hash, buffer_binding.buffer, buffer_binding.size);

		for (auto& dynamic_buffer_binding : other.dynamic_buffer_bindings)
			AddDynamicBufferBinding(dynamic_buffer_binding.name_hash, dynamic_buffer_binding.constant_buffer);
	}

	DescriptorSetBindings::DescriptorSetBindings(const Device::ResourceBindings& resource_bindings, const ShaderProgram::DescriptorSetLayout& descriptor_set_layout)
		: descriptor_set_layout(descriptor_set_layout)
	{
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
			case ShaderProgram::BindingType::StorageBuffer:
			{
				if (auto buffer_binding = resource_bindings.GetBufferBinding(binding.name_hash))
				{
					vk::Buffer buffer = buffer_binding->buffer->Buffer();
					const size_t size = buffer_binding->size;
					const uint32_t dynamic_offset = GetShaderBufferHasDynamicOffset(buffer_binding->name_hash) ? buffer_binding->dynamic_offset : -1;

					AddBufferBinding(address.binding, 0, size, buffer, dynamic_offset);
					break;
				}

				throw std::runtime_error("Buffer required by shader is missing in the ResourceBindings");
			}

			case ShaderProgram::BindingType::UniformBufferDynamic:
			case ShaderProgram::BindingType::StorageBufferDynamic:
			{
				if (auto buffer_binding = resource_bindings.GetDynamicBufferBinding(binding.name_hash))
				{
					auto constant_buffer = buffer_binding->constant_buffer;
					AddBufferBinding(address.binding, 0, binding.size, constant_buffer->GetBuffer()->Buffer(), 0);
					AddDynamicBufferBinding(address.binding, binding.name_hash, binding.size, &binding, constant_buffer);
					break;
				}

				// Default constant buffer for uniforms
				if (binding.type == ShaderProgram::BindingType::UniformBufferDynamic)
				{
					auto constant_buffer = Engine::Get()->GetSceneRenderer()->GetSceneBuffers()->GetConstantBuffer();
					AddBufferBinding(address.binding, 0, binding.size, constant_buffer->GetBuffer()->Buffer(), 0);
					AddDynamicBufferBinding(address.binding, binding.name_hash, binding.size, &binding, constant_buffer);
					break;
				}

				throw std::runtime_error("Dynamic buffer required by shader is missing in the ResourceBindings");
			}

			case ShaderProgram::BindingType::Sampler: break; // don't accumulate samplers

			default:
				throw std::runtime_error("unknown shader binding");
			}
		}

		std::sort(buffer_bindings.begin(), buffer_bindings.end());
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
	}

	void DescriptorSetBindings::AddDynamicBufferBinding(unsigned index, uint32_t name_hash, size_t size, const ShaderProgram::BindingData* binding_data, ConstantBuffer* constant_buffer)
	{
		dynamic_buffer_bindings.push_back(DynamicBufferBinding{ index, name_hash, (uint32_t)size, binding_data, constant_buffer });
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
		case ShaderProgram::BindingType::StorageBufferDynamic:
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

	uint32_t DescriptorSetBindings::DynamicBufferBinding::FlushConstantBuffer(const ConstantBindings& constants) const
	{
		assert(constant_buffer);
		assert(binding_data);
		auto data = constant_buffer->Allocate(size);
		memset(data.pointer, 0, data.size); // Fill with zeros in case any parameters are not set

		for (auto& member : binding_data->members)
		{
			auto& constant_bindings = constants.GetBindings();
			auto it = std::find_if(constant_bindings.begin(), constant_bindings.end(), [&member](const ConstantBindings::Binding& b) { return b.name_hash == member.name_hash; });
			if (it == constant_bindings.end())
				continue;

			memcpy(data.pointer + member.offset, it->data, std::min(member.size, it->size));
		}

		return data.offset;
	}

}