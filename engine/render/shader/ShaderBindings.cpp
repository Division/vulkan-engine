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
	}

	void ShaderBindings::Clear()
	{
		texture_bindings.clear();
		buffer_bindings.clear();
	}

} }