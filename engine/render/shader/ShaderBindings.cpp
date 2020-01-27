#include "render/texture/Texture.h"
#include "render/buffer/VulkanBuffer.h"

#include "ShaderBindings.h"

namespace core { namespace Device {

	void ShaderBindings::AddTextureBinding(unsigned set, unsigned index, Texture* texture)
	{
		texture_bindings.push_back(TextureBinding{ (unsigned char)set, (unsigned char)index, texture });
	}

	void ShaderBindings::AddBufferBinding(unsigned set, unsigned index, size_t offset, size_t size, vk::Buffer buffer)
	{
		buffer_bindings.push_back(BufferBinding{ (unsigned char)set, (unsigned char)index, (unsigned)offset, (unsigned)size, buffer });
		std::sort(buffer_bindings.begin(), buffer_bindings.end());
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
			{
				auto it = std::find_if(buffer_bindings.begin(), buffer_bindings.end(), predicate);
				return it != buffer_bindings.end() ? it - buffer_bindings.begin() : -1;
			}
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