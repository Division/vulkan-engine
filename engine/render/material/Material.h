#pragma once

#include "CommonIncludes.h"
#include "render/shader/ShaderCache.h"
#include "render/shader/ShaderBindings.h"
#include "render/shader/ShaderCaps.h"
#include "render/device/Resource.h"
#include "render/texture/Texture.h"
#include "resources/TextureResource.h"
#include "Handle.h"
#include "render/renderer/IRenderer.h"
#include <gsl/span>

#include <variant>

namespace Device 
{
	class Texture;
	class VulkanBuffer;
}

namespace render
{
	// Stores list of constants in a single allocation (or even 0 for small ones)
	class ConstantBindingStorage
	{
	public:
		struct ConstantData
		{
			const void* data;
			uint32_t name_hash;
			uint16_t size;
		};

		void AddFloatConstant(const char* name, float value);
		std::optional<float> GetFloatConstant(const char* name);
		void AddFloat2Constant(const char* name, vec2 value);
		std::optional<vec2> GetFloat2Constant(const char* name);
		void AddFloat3Constant(const char* name, vec3 value);
		std::optional<vec3> GetFloat3Constant(const char* name);
		void AddFloat4Constant(const char* name, vec4 value);
		std::optional<float4> GetFloat4Constant(const char* name);
		void RemoveConstant(const char* name);
		void RemoveConstant(uint32_t hash);
		uint32_t GetConstantCount() const { return constant_count; }
		ConstantData GetConstantByIndex(uint32_t index) const;
		void Clear();
		void Flush(Device::ConstantBindings& bindings) const;

	private:
		template<typename T>
		std::optional<T> GetConstantValue(const char* name);

		template<typename T>
		void AddConstant(uint32_t hash, T value);

		template<typename T>
		T* FindConstant(uint32_t hash);

		void* GetConstantDataByOffset(size_t offset);

		struct ConstantMetadata;
		gsl::span<ConstantMetadata> GetMetadata() const;
		gsl::span<uint8_t> GetConstantsData() const;

		uint16_t constant_count = 0;
		mutable utils::SmallVector<uint8_t, 64> data;
	};
}

class Material : public Common::Resource {
public:
	friend class SceneRenderer;
	friend class PassRenderer;

	using Handle = Common::Handle<Material>;

	static Handle Create()
	{
		return Handle(std::make_unique<Material>());
	}

	Material(const Material&) = default;
	Material();
	Material(const std::wstring& shader_path);
	~Material();

	Handle Clone() const;

	void Texture0(Device::Handle<Device::Texture> texture);
	const Device::Handle<Device::Texture>& Texture0() const { return texture0; };

	void SetShaderPath(std::wstring shader_path);
	const std::wstring& GetShaderPath() const { return shader_path; };

	void SetTexture0Resource(Resources::TextureResource::Handle texture);
	const Resources::TextureResource::Handle& GetTexture0Resource() const { return texture0_resource; };

	Device::Texture* GetTexture0() const;

	void NormalMap(Device::Handle<Device::Texture> normal_map_);
	Device::Handle<Device::Texture> NormalMap() const { return normal_map; };

	void SetNormalMapResource(Resources::TextureResource::Handle normal_map);
	const Resources::TextureResource::Handle& GetNormalMapResource() const { return normal_map_resource; };

	void AddExtraTexture(Resources::TextureResource::Handle texture, const char* name);
	void AddExtraTexture(Device::Handle<Device::Texture> texture, const char* name);
	void AddExtraBuffer(Device::Handle<Device::VulkanBuffer> buffer, const char* name);
	void ClearExtraResources();

	Device::Texture* GetNormalMap() const;

	void LightingEnabled(bool lighting_enabled_);
	bool LightingEnabled() const { return lighting_enabled; }

	// TODO: remove from material, mesh property
	void VertexColorEnabled(bool vertex_color_enabled_);
	bool VertexColorEnabled() const { return vertex_color_enabled; }
	
	float GetRoughness() const { return roughness; }
	void SetRoughness(float value) { roughness = value; }

	float GetMetalness() const { return metalness; }
	void SetMetalness(float value) { metalness = value; }

	vec4 GetColor() const { return color; }
	void SetColor(vec4 value) { color = value; }

	RenderQueue GetRenderQueue() const { return render_queue; }
	void SetRenderQueue(RenderQueue value) { render_queue = value; }

	void SetAlphaCutoff(bool value);
	bool GetAlphaCutoff() const { return alpha_cutoff; }

	void AddFloatConstant(const char* name, float value);
	void AddFloat2Constant(const char* name, vec2 value);
	void AddFloat3Constant(const char* name, vec3 value);
	void AddFloat4Constant(const char* name, vec4 value);
	void ClearConstants();

	const ShaderCapsSet& ShaderCaps() const { if (caps_dirty) UpdateCaps(); return shader_caps; }
	const Device::ResourceBindings& GetResourceBindings() const { if (bindings_dirty) UpdateBindings(); return resource_bindings; }
	const Device::ConstantBindings& GetConstantBindings() const { if (constants_dirty) UpdateConstants(); return constant_bindings; }

	const Device::ShaderProgramInfo& GetShaderInfo() const { UpdateShaderHash(); return shader_info; }
	const Device::ShaderProgramInfo& GetDepthOnlyShaderInfo() const { UpdateShaderHash(); return depth_only_shader_info; }

	uint32_t GetHash() const;

protected:
	void UpdateCaps() const;
	void UpdateBindings() const;
	void UpdateConstants() const;
	void UpdateShaderHash() const;
	void SetDirty();
	void SetBindingsDirty();
	void SetConstantsDirty();

	struct ExtraResourceBinding
	{
		uint32_t name_hash;
		std::variant<Resources::TextureResource::Handle, Device::Handle<Device::Texture>, Device::Handle<Device::VulkanBuffer>> resource;
	};

	void AddExtraResourceBinding(const ExtraResourceBinding& value);

protected:
	render::ConstantBindingStorage constant_storage;

	mutable bool caps_dirty = true;
	mutable bool shader_hash_dirty = true;
	std::wstring shader_path = L"shaders/material_main.hlsl";
	std::string vs_entry = "vs_main";
	std::string ps_entry = "ps_main";
	mutable ShaderCapsSet shader_caps;

	mutable bool bindings_dirty = true;
	mutable Device::ResourceBindings resource_bindings;
	std::vector<ExtraResourceBinding> extra_resource_bindings;

	mutable bool constants_dirty = true;
	mutable Device::ConstantBindings constant_bindings;

	vec4 color = vec4(1,1,1,1);
	float roughness = 0.5;
	float metalness = 0.2;
	bool alpha_cutoff = false;

	mutable Device::ShaderProgramInfo shader_info;
	mutable Device::ShaderProgramInfo depth_only_shader_info;

	bool has_texture0 = false;
	Device::Handle<Device::Texture> texture0;
	Resources::TextureResource::Handle texture0_resource;

	bool has_normal_map = false;
	Device::Handle<Device::Texture> normal_map;
	Resources::TextureResource::Handle normal_map_resource;

	bool lighting_enabled = true;
	bool vertex_color_enabled = false;
	RenderQueue render_queue = RenderQueue::Opaque;
};

namespace render
{
	class MaterialList : public std::vector<Material::Handle>, public Common::Resource
	{
	public:
		using Handle = std::shared_ptr<MaterialList>;

		static Handle Create()
		{
			return Handle(std::make_unique<MaterialList>());
		}
	};
}