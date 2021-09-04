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
	class ConstantBuffer;

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

	class ConstantBindings
	{
	public:
		struct Binding
		{
			void* data = nullptr;
			uint32_t size = 0;
			uint32_t name_hash = 0;
		};

		Binding* GetBinding(const uint32_t name_hash)
		{
			auto it = std::find_if(bindings.begin(), bindings.end(), [name_hash](const Binding& binding) { return binding.name_hash == name_hash; });
			return it == bindings.end() ? nullptr : &*it;
		}

		void AddDataBinding(void* data, uint32_t size, const char* name)
		{
			AddDataBinding(data, size, ShaderProgram::GetParameterNameHash(name));
		}

		void AddDataBinding(void* data, uint32_t size, uint32_t name_hash)
		{
			Binding* binding = GetBinding(name_hash);
			if (!binding)
			{
				bindings.push_back({});
				binding = &bindings[bindings.size() - 1];
			}
			binding->data = data;
			binding->size = size;
			binding->name_hash = name_hash;
		}

		const auto& GetBindings() const { return bindings; }

		void Merge(const ConstantBindings& other)
		{
			for (auto& binding : other.GetBindings())
				AddDataBinding(binding.data, binding.size, binding.name_hash);
		}

		void AddFloat4Binding(vec4* data, uint32_t name_hash) { AddDataBinding(data, sizeof(*data), name_hash); }
		void AddFloat4Binding(vec4* data, const char* name) { AddFloat4Binding(data, ShaderProgram::GetParameterNameHash(name)); }
		void AddFloat3Binding(vec3* data, uint32_t name_hash) { AddDataBinding(data, sizeof(*data), name_hash); }
		void AddFloat3Binding(vec3* data, const char* name) { AddFloat3Binding(data, ShaderProgram::GetParameterNameHash(name)); }
		void AddFloat2Binding(vec2* data, uint32_t name_hash) { AddDataBinding(data, sizeof(*data), name_hash); }
		void AddFloat2Binding(vec2* data, const char* name) { AddFloat2Binding(data, ShaderProgram::GetParameterNameHash(name)); }
		void AddFloatBinding(float* data, uint32_t name_hash) { AddDataBinding(data, sizeof(*data), name_hash); }
		void AddFloatBinding(float* data, const char* name) { AddFloatBinding(data, ShaderProgram::GetParameterNameHash(name)); }
		void AddFloat4x4Binding(mat4* data, uint32_t name_hash) { AddDataBinding(data, sizeof(*data), name_hash); }
		void AddFloat4x4Binding(mat4* data, const char* name) { AddFloat4x4Binding(data, ShaderProgram::GetParameterNameHash(name)); }
		void AddFloat3x3Binding(mat3* data, uint32_t name_hash) { AddDataBinding(data, sizeof(*data), name_hash); }
		void AddFloat3x3Binding(mat3* data, const char* name) { AddFloat3x3Binding(data, ShaderProgram::GetParameterNameHash(name)); }


	private:
		//std::vector<Binding> bindings;
		utils::SmallVector<Binding, 16> bindings;
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

		struct DynamicBufferBinding
		{
			uint32_t index = 0;
			uint32_t name_hash = 0;
			uint32_t size = 0;
			const ShaderProgram::BindingData* binding_data = nullptr;
			ConstantBuffer* constant_buffer = nullptr;

			uint32_t FlushConstantBuffer(const ConstantBindings& constants) const;
		};

		DescriptorSetBindings(const Device::ResourceBindings& resource_bindings, const ShaderProgram::DescriptorSetLayout& descriptor_set_layout);
		DescriptorSetBindings(const ShaderProgram::DescriptorSetLayout& descriptor_set_layout);

		const auto& GetTextureBindings() const { return texture_bindings; }
		auto& GetTextureBindings() { return texture_bindings; }
		const auto& GetBufferBindings() const { return buffer_bindings; }
		auto& GetBufferBindings() { return buffer_bindings; }
		const auto& GetDynamicBufferBindings() const { return dynamic_buffer_bindings; }
		auto& GetDynamicBufferBindings() { return dynamic_buffer_bindings; }
		const auto& GetDynamicOffsets() const { return dynamic_offsets; }

		void SetupWithResourceBindings(const Device::ResourceBindings& resource_bindings, const ShaderProgram::DescriptorSetLayout& descriptor_set_layout);

		void AddTextureBindingSafe(unsigned index, const Texture* texture);
		void AddBufferBindingSafe(unsigned index, size_t offset, size_t size, vk::Buffer buffer);
		void AddTextureBinding(unsigned index, const Texture* texture);
		void AddBufferBinding(unsigned index, size_t offset, size_t size, vk::Buffer buffer, size_t dynamic_offset = -1);
		void AddDynamicBufferBinding(unsigned index, uint32_t name_hash, size_t size, const ShaderProgram::BindingData* binding_data, ConstantBuffer* constant_buffer);
		void Clear();
		void UpdateBindings();
		
		const ShaderProgram::DescriptorSetLayout& GetDescriptorSetLayout() const { return descriptor_set_layout; };

	private:
		int GetBindingIndex(uint32_t index, ShaderProgram::BindingType type);

		const ShaderProgram::DescriptorSetLayout& descriptor_set_layout;
		/*std::vector<uint32_t> dynamic_offsets;
		std::vector<TextureBinding> texture_bindings;
		std::vector<BufferBinding> buffer_bindings;
		std::vector<DynamicBufferBinding> dynamic_buffer_bindings;*/

		utils::SmallVector<uint32_t, 8> dynamic_offsets;
		utils::SmallVector<TextureBinding, 32> texture_bindings;
		utils::SmallVector<BufferBinding, 16> buffer_bindings;
		utils::SmallVector<DynamicBufferBinding, 16> dynamic_buffer_bindings;
	};


}