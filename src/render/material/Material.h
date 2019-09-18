#pragma once

#include "CommonIncludes.h"
#include "render/shader/ShaderCaps.h"

class Texture;

class Material {
public:
	friend class SceneRenderer;
	friend class PassRenderer;

	Material() = default;

	void texture0(std::shared_ptr<Texture> texture);
	const std::shared_ptr<Texture>& texture0() const { return _texture0; };
	
	void normalMap(std::shared_ptr<Texture> normalMap);
	std::shared_ptr<Texture> normalMap() const { return _normalMap; };

	void lightingEnabled(bool lightingEnabled);
	bool lightingEnabled() const { return _lightingEnabled; }

	void vertexColorEnabled(bool vertexColorEnabled);
	bool vertexColorEnabled() const { return _vertexColorEnabled; }
	
	const ShaderCapsSet &shaderCaps() const { return _shaderCaps; }
	const ShaderCapsSet &shaderCapsSkinning() const { return _shaderCapsSkinning; }

protected:
	void _updateCaps();
	void _capsInitialized();

protected:
	static std::unordered_set<ShaderCapsSet::Bitmask> _capsVariations;
	static std::vector<ShaderCapsSet::Bitmask> _uninitializedCaps;

	bool _capsDirty = true;
	ShaderCapsSet _shaderCaps;
	ShaderCapsSet _shaderCapsSkinning;

	bool _hasObjectParams = true;

	bool _hasTexture0 = false;
	std::shared_ptr<Texture> _texture0;

	bool _hasNormalMap = false;
	std::shared_ptr<Texture> _normalMap;

	bool _lightingEnabled = true;
	bool _vertexColorEnabled = false;
};