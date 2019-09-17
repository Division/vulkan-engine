#pragma once

#include <map>
#include <string>

enum class ShaderResourceName : uint32_t {
	Texture0,
	Texture1,
	NormalMap,
	SpecularMap,
	ShadowMap,
	ProjectorTexture,
	LightGrid,
	LightIndices
};

enum class ConstantBufferName : uint32_t {
	ObjectParams,
	Camera,
	SkinningMatrices,
	Light,
	Projector
};

extern const std::map<std::string, ShaderResourceName> SHADER_SAMPLER_NAMES;
