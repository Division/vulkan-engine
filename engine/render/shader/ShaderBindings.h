#pragma once

#include "CommonIncludes.h"
#include "utils/DataStructures.h"
#include "Shader.h"
#include <optional>

namespace render
{
	class SceneRenderer;
}

namespace Device {

	class VulkanBuffer;
	class Texture;

	// User code uses this to specify bindings (e.g. textures, buffers) by their name (hash)
	class ResourceBindings
	{
	public:
		struct Base
		{
			uint32_t name_hash = 0;
		};

		struct TextureResourceBinding : public Base
		{
			Texture* texture = nullptr;
		};

		struct BufferResourceBinding : public Base
		{
			VulkanBuffer* buffer = nullptr;
			uint32_t size = 0;
			uint32_t dynamic_offset = 0;
		};
		
		void AddTextureBinding(const std::string& name, Texture* texture);
		void AddTextureBinding(uint32_t name_hash, Texture* texture);
		Texture* GetTextureBinding(uint32_t name_hash) const;

		void AddBufferBinding(const std::string& name, VulkanBuffer* buffer, uint32_t size, uint32_t dynamic_offset = 0);
		void AddBufferBinding(uint32_t name_hash, VulkanBuffer* buffer, uint32_t size, uint32_t dynamic_offset = 0);
		std::optional<BufferResourceBinding> GetBufferBinding(uint32_t name_hash) const;

		void Clear();
		void Merge(const ResourceBindings& other);

	private:
		uint32_t GetTextureBindingIndex(uint32_t name_hash) const;
		uint32_t GetBufferBindingIndex(uint32_t name_hash) const;

		//std::vector<TextureResourceBinding> texture_bindings;
		//std::vector<BufferResourceBinding> buffer_bindings;

		utils::SmallVector<TextureResourceBinding, 32> texture_bindings;
		utils::SmallVector<BufferResourceBinding, 16> buffer_bindings;
	};

	// Represents bindings for a single descriptor set
	// Used by rendering. Contains actual shader binding indices.
	class DescriptorSetBindings
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

		DescriptorSetBindings(const Device::ResourceBindings& resource_bindings, const ShaderProgram::DescriptorSetLayout& descriptor_set_layout);
		DescriptorSetBindings(const ShaderProgram::DescriptorSetLayout& descriptor_set_layout);

		const auto& GetTextureBindings() const { return texture_bindings; }
		auto& GetTextureBindings() { return texture_bindings; }
		const auto& GetBufferBindings() const { return buffer_bindings; }
		auto& GetBufferBindings() { return buffer_bindings; }
		const auto& GetDynamicOffsets() const { return dynamic_offsets; }

		void SetupWithResourceBindings(const Device::ResourceBindings& resource_bindings, const ShaderProgram::DescriptorSetLayout& descriptor_set_layout);

		void AddTextureBindingSafe(unsigned index, const Texture* texture);
		void AddBufferBindingSafe(unsigned index, size_t offset, size_t size, vk::Buffer buffer);
		void AddTextureBinding(unsigned index, const Texture* texture);
		void AddBufferBinding(unsigned index, size_t offset, size_t size, vk::Buffer buffer, size_t dynamic_offset = -1);
		int GetBindingIndex(uint32_t index, ShaderProgram::BindingType type);
		void Clear();
		void UpdateBindings();
		
		const ShaderProgram::DescriptorSetLayout& GetDescriptorSetLayout() const { return descriptor_set_layout; };

	private:
		const ShaderProgram::DescriptorSetLayout& descriptor_set_layout;
		/*std::vector<uint32_t> dynamic_offsets;
		std::vector<TextureBinding> texture_bindings;
		std::vector<BufferBinding> buffer_bindings;*/

		utils::SmallVector<uint32_t, 8> dynamic_offsets;
		utils::SmallVector<TextureBinding, 32> texture_bindings;
		utils::SmallVector<BufferBinding, 16> buffer_bindings;
	};


}