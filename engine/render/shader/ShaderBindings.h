#pragma once

#include "CommonIncludes.h"
#include "utils/DataStructures.h"
#include "Shader.h"

namespace core { namespace Device {

	class VulkanBuffer;
	class Texture;

	// Represents bindings for a single descriptor set
	class ShaderBindings
	{
	public:
		struct TextureBinding
		{
			unsigned char index;
			const Texture* texture;
		};

		struct BufferBinding
		{
			unsigned char index;
			unsigned int offset;
			unsigned int dynamic_offset;
			unsigned int size;
			vk::Buffer buffer;
			friend bool operator<(const BufferBinding& a, const BufferBinding& b) { return a.index < b.index; }
		};

		const utils::SmallVectorBase<TextureBinding>& GetTextureBindings() const { return texture_bindings; }
		utils::SmallVectorBase<TextureBinding>& GetTextureBindings() { return texture_bindings; }
		const utils::SmallVectorBase<BufferBinding>& GetBufferBindings() const { return buffer_bindings; }
		utils::SmallVectorBase<BufferBinding>& GetBufferBindings() { return buffer_bindings; }
		const auto& GetDynamicOffsets() const { return dynamic_offsets; }

		void AddTextureBindingSafe(unsigned index, const Texture* texture);
		void AddBufferBindingSafe(unsigned index, size_t offset, size_t size, vk::Buffer buffer);
		void AddTextureBinding(unsigned index, const Texture* texture);
		void AddBufferBinding(unsigned index, size_t offset, size_t size, vk::Buffer buffer, size_t dynamic_offset = -1);
		int GetBindingIndex(uint32_t index, ShaderProgram::BindingType type);
		void Clear();
		void UpdateBindings();
	private:
		utils::SmallVector<uint32_t, 4> dynamic_offsets;
		utils::SmallVector<TextureBinding, 6> texture_bindings;
		utils::SmallVector<BufferBinding, 10> buffer_bindings;
	};


} }