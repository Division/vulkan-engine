#pragma once

#include "CommonIncludes.h"

namespace core { namespace Device {

	class VulkanBuffer;
	class Texture;

	class ShaderBindings
	{
	public:
		struct TextureBinding
		{
			unsigned set;
			unsigned index;
			Texture* texture;
		};

		struct BufferBinding
		{
			unsigned set;
			unsigned index;
			size_t size;
			vk::Buffer buffer;
		};

		const std::vector<TextureBinding>& GetTextureBindings() const { return texture_bindings; }
		const std::vector<BufferBinding>& GetBufferBindings() const { return buffer_bindings; }

		void AddTextureBinding(unsigned set, unsigned index, Texture* texture);
		void AddBufferBinding(unsigned set, unsigned index, size_t size, vk::Buffer buffer);
		void Clear();

	private:

		std::vector<TextureBinding> texture_bindings;
		std::vector<BufferBinding> buffer_bindings;
	};


} }