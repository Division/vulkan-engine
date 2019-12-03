#include "Material.h"
#include "render/texture/Texture.h"
#include "render/shader/ShaderCache.h"

std::unordered_set<ShaderCapsSet::Bitmask> Material::_capsVariations;
std::vector<ShaderCapsSet::Bitmask> Material::_uninitializedCaps;

Material::Material()
{
	vertex_hash = ShaderCache::GetShaderPathHash(shader_path + L".vert");
	vertex_hash_depth_only = ShaderCache::GetShaderPathHash(shader_path + L".vert", { "DEPTH_ONLY" });
	fragment_hash = ShaderCache::GetShaderPathHash(shader_path + L".frag");
}

void Material::texture0(std::shared_ptr<Texture> texture) {
	_texture0 = texture;
	
	if ((bool)texture != _hasTexture0) {
		_hasTexture0 = (bool)texture;
		_capsDirty = true;
	}
}

void Material::normalMap(std::shared_ptr<Texture> normalMap) {
	_normalMap = normalMap;

	if ((bool)normalMap != _hasNormalMap) {
		_hasNormalMap = (bool)normalMap;
		_capsDirty = true;
	}
}

void Material::lightingEnabled(bool lightingEnabled) {
	if (lightingEnabled != _lightingEnabled) {
		_lightingEnabled = lightingEnabled;
		_capsDirty = true;
	}
}

void Material::vertexColorEnabled(bool vertexColorEnabled) {
	if (vertexColorEnabled != _vertexColorEnabled) {
		_vertexColorEnabled = vertexColorEnabled;
		_capsDirty = true;
	}
}

void Material::_updateCaps() {
	if (!_capsDirty) { return; }

	if (_hasTexture0) {
		_shaderCaps.addCap(ShaderCaps::Texture0);
	} else {
		_shaderCaps.removeCap(ShaderCaps::Texture0);
	}

	if (_hasNormalMap) {
		_shaderCaps.addCap(ShaderCaps::NormalMap);
	} else {
		_shaderCaps.removeCap(ShaderCaps::NormalMap);
	}

	if (_lightingEnabled) {
		_shaderCaps.addCap(ShaderCaps::Lighting);
	} else {
		_shaderCaps.removeCap(ShaderCaps::Lighting);
	}

	if (_vertexColorEnabled) {
		_shaderCaps.addCap(ShaderCaps::VertexColor);
	} else {
		_shaderCaps.removeCap(ShaderCaps::VertexColor);
	}

	auto bitmask = _shaderCaps.getBitmask();
	if (!_capsVariations.count(bitmask)) {
		_capsVariations.insert(bitmask);
		_uninitializedCaps.push_back(bitmask);
	}

	_shaderCapsSkinning = _shaderCaps;
	_shaderCapsSkinning.addCap(ShaderCaps::Skinning);

	_capsDirty = false;
}

void Material::_capsInitialized() {
	_uninitializedCaps.clear();
}