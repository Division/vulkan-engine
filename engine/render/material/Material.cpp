#include "Material.h"
#include "render/texture/Texture.h"
#include "render/shader/ShaderCache.h"
#include "utils/Math.h"

Material::Material()
{
	vertex_hash = Device::ShaderCache::GetShaderPathHash(shader_path + L".vert");
	vertex_hash_depth_only = Device::ShaderCache::GetShaderPathHash(shader_path + L".vert", { "DEPTH_ONLY" });
	fragment_hash = Device::ShaderCache::GetShaderPathHash(shader_path + L".frag");
}

void Material::texture0(std::shared_ptr<Device::Texture> texture) {
	_texture0 = texture;
	
	if ((bool)texture != _hasTexture0) {
		_hasTexture0 = (bool)texture;
		_capsDirty = true;
	}
}

void Material::normalMap(std::shared_ptr<Device::Texture> normalMap) {
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

void Material::_updateCaps() const {
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

	_shaderCapsSkinning = _shaderCaps;
	_shaderCapsSkinning.addCap(ShaderCaps::Skinning);

	_capsDirty = false;
}

uint32_t Material::GetHash() const
{
	uint32_t hashes[] = {
		vertex_hash,
		fragment_hash,
		_shaderCaps.getBitmask(),
		_texture0 ? _texture0->GetHash() : 0,
		_normalMap ? _normalMap->GetHash() : 0,
		_lightingEnabled,
		_vertexColorEnabled,
		FastHash(&roughness, sizeof(roughness))
	};

	return FastHash(hashes, sizeof(hashes));
}
