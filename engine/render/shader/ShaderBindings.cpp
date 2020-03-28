#include "render/texture/Texture.h"
#include "render/buffer/VulkanBuffer.h"

#include "ShaderBindings.h"

namespace core { namespace Device {

	void ShaderBindings::AddTextureBindingSafe(ShaderProgram::BindingAddress address, const Texture* texture)
	{
		if (address.set == -1 || address.binding == -1)
			return;

		AddTextureBinding(address.set, address.binding, texture);
	}

	void ShaderBindings::AddBufferBindingSafe(ShaderProgram::BindingAddress address, size_t offset, size_t size, vk::Buffer buffer)
	{
		if (address.set == -1 || address.binding == -1)
			return;

		AddBufferBinding(address.set, address.binding, offset, size, buffer);
	}

	void ShaderBindings::AddTextureBinding(unsigned set, unsigned index, const Texture* texture)
	{
		texture_bindings.push_back(TextureBinding{ (unsigned char)set, (unsigned char)index, texture });
	}

	void ShaderBindings::AddBufferBinding(unsigned set, unsigned index, size_t offset, size_t size, vk::Buffer buffer, size_t dynamic_offset)
	{
		buffer_bindings.push_back(BufferBinding{ (unsigned char)set, (unsigned char)index, (unsigned)offset, (unsigned)dynamic_offset, (unsigned)size, buffer });
		UpdateBindings();
	}

	void ShaderBindings::UpdateBindings()
	{
		std::sort(buffer_bindings.begin(), buffer_bindings.end());
		
		dynamic_offsets.clear();
		for (auto& binding : buffer_bindings)
			if (binding.dynamic_offset != -1)
				dynamic_offsets.push_back(binding.dynamic_offset);
	}

	int ShaderBindings::GetBindingIndex(uint32_t index, ShaderProgram::BindingType type)
	{
		auto predicate = [index](auto& binding)
		{
			return binding.index == index;
		};

		switch (type)
		{
		case ShaderProgram::BindingType::Sampler:
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
		
		default:
			assert(false);
		}

		return -1;
	}

	void ShaderBindings::Clear()
	{
		texture_bindings.clear();
		buffer_bindings.clear();
		dynamic_offsets.clear();
	}

} }