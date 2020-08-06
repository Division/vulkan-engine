#pragma once

#include "CommonIncludes.h"
#include "render/shader/ShaderCaps.h"

namespace Device 
{
	class Texture;
}

class Material {
public:
	friend class SceneRenderer;
	friend class PassRenderer;

	Material(const Material&) = default;
	Material();

	void texture0(std::shared_ptr<Device::Texture> texture);
	const std::shared_ptr<Device::Texture>& texture0() const { return _texture0; };
	
	void normalMap(std::shared_ptr<Device::Texture> normalMap);
	std::shared_ptr<Device::Texture> normalMap() const { return _normalMap; };

	void lightingEnabled(bool lightingEnabled);
	bool lightingEnabled() const { return _lightingEnabled; }

	void vertexColorEnabled(bool vertexColorEnabled);
	bool vertexColorEnabled() const { return _vertexColorEnabled; }
	
	float GetRoughness() const { return roughness; }
	void SetRoughness(float value) { roughness = value; }

	const ShaderCapsSet& shaderCaps() const { if (_capsDirty) _updateCaps(); return _shaderCaps; }
	const ShaderCapsSet &shaderCapsSkinning() const { if (_capsDirty) _updateCaps(); return _shaderCapsSkinning; }

	uint32_t GetVertexShaderNameHash() const { return vertex_hash; }
	uint32_t GetVertexShaderDepthOnlyNameHash() const { return vertex_hash_depth_only; }
	uint32_t GetFragmentShaderNameHash() const { return fragment_hash; }

	uint32_t GetHash() const;

protected:
	void _updateCaps() const;

protected:
	mutable bool _capsDirty = true;
	std::wstring shader_path = L"shaders/material_main";
	mutable ShaderCapsSet _shaderCaps;
	mutable ShaderCapsSet _shaderCapsSkinning;
	float roughness = 0;

	uint32_t vertex_hash_depth_only = 0;
	uint32_t vertex_hash = 0;
	uint32_t fragment_hash = 0;

	bool _hasObjectParams = true;

	bool _hasTexture0 = false;
	std::shared_ptr<Device::Texture> _texture0;

	bool _hasNormalMap = false;
	std::shared_ptr<Device::Texture> _normalMap;

	bool _lightingEnabled = true;
	bool _vertexColorEnabled = false;
};