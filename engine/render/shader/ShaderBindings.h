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
			const Texture* texture;
		};

		struct BufferBinding
		{
			unsigned char set;
			unsigned char index;
			unsigned int offset;
			unsigned int dynamic_offset;
			unsigned int size;
			vk::Buffer buffer;
			friend bool operator<(const BufferBinding& a, const BufferBinding& b) { return std::tie(a.set, a.index) < std::tie(b.set, b.index); }
		};

		const utils::SmallVectorBase<TextureBinding>& GetTextureBindings() const { return texture_bindings; }
		utils::SmallVectorBase<TextureBinding>& GetTextureBindings() { return texture_bindings; }
		const utils::SmallVectorBase<BufferBinding>& GetBufferBindings() const { return buffer_bindings; }
		utils::SmallVectorBase<BufferBinding>& GetBufferBindings() { return buffer_bindings; }
		const auto& GetDynamicOffsets() const { return dynamic_offsets; }

		void AddTextureBindingSafe(ShaderProgram::BindingAddress address, const Texture* texture);
		void AddBufferBindingSafe(ShaderProgram::BindingAddress address, size_t offset, size_t size, vk::Buffer buffer);
		void AddTextureBinding(unsigned set, unsigned index, const Texture* texture);
		void AddBufferBinding(unsigned set, unsigned index, size_t offset, size_t size, vk::Buffer buffer, size_t dynamic_offset = -1);
		int GetBindingIndex(uint32_t index, ShaderProgram::BindingType type);
		void Clear();
		void UpdateBindings();
	private:
		utils::SmallVector<uint32_t, 4> dynamic_offsets;
		utils::SmallVector<TextureBinding, 6> texture_bindings;
		utils::SmallVector<BufferBinding, 10> buffer_bindings;
	};


} }