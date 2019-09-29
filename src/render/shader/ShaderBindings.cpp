#include "render/texture/Texture.h"
#include "render/buffer/VulkanBuffer.h"

#include "ShaderBindings.h"

namespace core { namespace Device {

	void ShaderBindings::AddTextureBinding(unsigned set, unsigned index, Texture* texture)
	{
		texture_bindings.emplace_back(TextureBinding{ set, index, texture });
	}

	void ShaderBindings::AddBufferBinding(unsigned set, unsigned index, size_t size, vk::Buffer buffer)
	{
		buffer_bindings.emplace_back(BufferBinding{ set, index, size, buffer });
	}

	void ShaderBindings::Clear()
	{
		texture_bindings.clear();
		buffer_bindings.clear();
	}

} }