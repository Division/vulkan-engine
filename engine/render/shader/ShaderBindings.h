#pragma once

#include "CommonIncludes.h"
#include "utils/DataStructures.h"
#include "Shader.h"

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
			friend bool operator<(const BufferBinding& a, const BufferBinding& b) { return a.index < b.index; }
		};

		/*const std::vector<TextureBinding>& GetTextureBindings() const { return texture_bindings; }
		std::vector<TextureBinding>& GetTextureBindings() { return texture_bindings; }
		const std::vector<BufferBinding>& GetBufferBindings() const { return buffer_bindings; }
		std::vector<BufferBinding>& GetBufferBindings() { return buffer_bindings; }*/

		const utils::SmallVectorBase<TextureBinding>& GetTextureBindings() const { return texture_bindings; }
		utils::SmallVectorBase<TextureBinding>& GetTextureBindings() { return texture_bindings; }
		const utils::SmallVectorBase<BufferBinding>& GetBufferBindings() const { return buffer_bindings; }
		utils::SmallVectorBase<BufferBinding>& GetBufferBindings() { return buffer_bindings; }
		
		void AddTextureBinding(unsigned set, unsigned index, Texture* texture);
		void AddBufferBinding(unsigned set, unsigned index, size_t offset, size_t size, vk::Buffer buffer);
		int GetBindingIndex(uint32_t index, ShaderProgram::BindingType type);
		void Clear();

	private:
		/*std::vector<uint32_t> dynamic_offsets;
		std::vector<TextureBinding> texture_bindings;
		std::vector<BufferBinding> buffer_bindings;*/

		utils::SmallVector<uint32_t, 10> dynamic_offsets;
		utils::SmallVector<TextureBinding, 6> texture_bindings;
		utils::SmallVector<BufferBinding, 10> buffer_bindings;
	};


} }