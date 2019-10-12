#pragma once

#include "CommonIncludes.h"
#include "render/shader/ShaderCaps.h"

namespace core 
{ 
	namespace Device 
	{
		class Texture;
	}
}

using namespace core::Device;

class Material {
public:
	friend class SceneRenderer;
	friend class PassRenderer;

	Material();

	void texture0(std::shared_ptr<Texture> texture);
	const std::shared_ptr<Texture>& texture0() const { return _texture0; };
	
	void normalMap(std::shared_ptr<Texture> normalMap);
	std::shared_ptr<Texture> normalMap() const { return _normalMap; };

	void lightingEnabled(bool lightingEnabled);
	bool lightingEnabled() const { return _lightingEnabled; }

	void vertexColorEnabled(bool vertexColorEnabled);
	bool vertexColorEnabled() const { return _vertexColorEnabled; }
	
	const ShaderCapsSet& shaderCaps() { if (_capsDirty) _updateCaps(); return _shaderCaps; }
	const ShaderCapsSet &shaderCapsSkinning() { if (_capsDirty) _updateCaps(); return _shaderCapsSkinning; }

	uint32_t GetVertexShaderNameHash() const { return vertex_hash; }
	uint32_t GetFragmentShaderNameHash() const { return fragment_hash; }

protected:
	void _updateCaps();
	void _capsInitialized();

protected:
	static std::unordered_set<ShaderCapsSet::Bitmask> _capsVariations;
	static std::vector<ShaderCapsSet::Bitmask> _uninitializedCaps;

	mutable bool _capsDirty = true;
	std::wstring shader_path = L"shaders/material_main";
	ShaderCapsSet _shaderCaps;
	ShaderCapsSet _shaderCapsSkinning;

	uint32_t vertex_hash = 0;
	uint32_t fragment_hash = 0;

	bool _hasObjectParams = true;

	bool _hasTexture0 = false;
	std::shared_ptr<Texture> _texture0;

	bool _hasNormalMap = false;
	std::shared_ptr<Texture> _normalMap;

	bool _lightingEnabled = true;
	bool _vertexColorEnabled = false;
};