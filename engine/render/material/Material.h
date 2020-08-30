﻿#pragma once

#include "CommonIncludes.h"
#include "render/shader/ShaderCaps.h"
#include "render/device/Resource.h"
#include "render/texture/Texture.h"
#include "render/shader/ShaderCache.h"
#include "Handle.h"

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
	~Material();

	void Texture0(Device::Handle<Device::Texture> texture);
	const Device::Handle<Device::Texture>& Texture0() const { return texture0; };
	
	void NormalMap(Device::Handle<Device::Texture> normal_map_);
	Device::Handle<Device::Texture> NormalMap() const { return normal_map; };

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

	const ShaderCapsSet& ShaderCaps() const { if (caps_dirty) UpdateCaps(); return shader_caps; }
	const ShaderCapsSet &ShaderCapsSkinning() const { if (caps_dirty) UpdateCaps(); return shader_caps_skinning; }

	const Device::ShaderProgramInfo& GetShaderInfo() const { UpdateShaderHash(); return shader_info; }
	const Device::ShaderProgramInfo& GetDepthOnlyShaderInfo() const { UpdateShaderHash(); return depth_only_shader_info; }

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
	mutable ShaderCapsSet shader_caps_skinning;
	vec4 color = vec4(1,1,1,1);
	float roughness = 0.5;
	float metalness = 0.2;

	mutable Device::ShaderProgramInfo shader_info;
	mutable Device::ShaderProgramInfo depth_only_shader_info;
	mutable uint32_t vertex_hash_depth_only = 0;
	mutable uint32_t vertex_hash = 0;
	mutable uint32_t fragment_hash = 0;

	bool has_texture0 = false;
	Device::Handle<Device::Texture> texture0;


	bool has_normal_map = false;
	Device::Handle<Device::Texture> normal_map;

	bool lighting_enabled = true;
	bool vertex_color_enabled = false;
};

namespace render
{
	class MaterialList : public std::vector<Material::Handle>, public Common::Resource
	{
	public:
		using Handle = Common::Handle<MaterialList>;

		static Handle Create()
		{
			return Handle(std::make_unique<MaterialList>());
		}
	};
}