#pragma once

#include "CommonIncludes.h"
#include "utils/DataStructures.h"

namespace core { namespace Device {

	class VulkanBuffer;
	class Texture;

	class ShaderBindings
	{
	public:
		struct TextureBinding
		{
			unsigned char set;
			unsigned char index;
			Texture* texture;
		};

		struct BufferBinding
		{
			unsigned char set;
			unsigned char index;
			unsigned int offset;
			unsigned int size;
			vk::Buffer buffer;
		};

		const utils::SmallVectorBase<TextureBinding, false>& GetTextureBindings() const { return texture_bindings; }
		const utils::SmallVectorBase<BufferBinding, false>& GetBufferBindings() const { return buffer_bindings; }

		void AddTextureBinding(unsigned set, unsigned index, Texture* texture);
		void AddBufferBinding(unsigned set, unsigned index, size_t offset, size_t size, vk::Buffer buffer);
		void Clear();

	private:

		utils::SmallVector<TextureBinding, 6, false> texture_bindings;
		utils::SmallVector<BufferBinding, 10, false> buffer_bindings;
	};


} }