﻿#pragma once

#include "CommonIncludes.h"
#include "render/shader/ShaderCaps.h"
#include "render/device/Resource.h"
#include "render/texture/Texture.h"
#include "render/shader/ShaderCache.h"
#include "resources/TextureResource.h"
#include "Handle.h"
#include "render/renderer/IRenderer.h"

namespace Device 
{
	class Texture;
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

	void SetTexture0Resource(Resources::TextureResource::Handle texture);
	const Resources::TextureResource::Handle& GetTexture0Resource() const { return texture0_resource; };

	void NormalMap(Device::Handle<Device::Texture> normal_map_);
	Device::Handle<Device::Texture> NormalMap() const { return normal_map; };

	void SetNormalMapResource(Resources::TextureResource::Handle normal_map);
	const Resources::TextureResource::Handle& GetNormalMapResource() const { return normal_map_resource; };

	void LightingEnabled(bool lighting_enabled_);
	bool LightingEnabled() const { return lighting_enabled; }

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

	const ShaderCapsSet& ShaderCaps() const { if (caps_dirty) UpdateCaps(); return shader_caps; }

	const Device::ShaderProgramInfo& GetShaderInfo() const { UpdateShaderHash(); return shader_info; }
	const Device::ShaderProgramInfo& GetDepthOnlyShaderInfo() const { UpdateShaderHash(); return depth_only_shader_info; }
	const Device::ShaderProgramInfo& GetShaderInfoSkinning() const { UpdateShaderHash(); return shader_info_skinning; }
	const Device::ShaderProgramInfo& GetDepthOnlyShaderInfoSkinning() const { UpdateShaderHash(); return depth_only_shader_info_skinning; }

	uint32_t GetHash() const;

protected:
	void UpdateCaps() const;
	void UpdateShaderHash() const;
	void SetDirty();

protected:
	mutable bool caps_dirty = true;
	mutable bool shader_hash_dirty = true;
	std::wstring shader_path = L"shaders/material_main.hlsl";
	mutable ShaderCapsSet shader_caps;
	vec4 color = vec4(1,1,1,1);
	float roughness = 0.5;
	float metalness = 0.2;

	// TODO: unify/factorize this when more combinations is needed
	mutable Device::ShaderProgramInfo shader_info;
	mutable Device::ShaderProgramInfo depth_only_shader_info;
	mutable Device::ShaderProgramInfo shader_info_skinning;
	mutable Device::ShaderProgramInfo depth_only_shader_info_skinning;

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