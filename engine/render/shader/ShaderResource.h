#pragma once

#include <map>
#include <string>

enum class ShaderTextureName : uint32_t 
{
	Texture0,
	Texture1,
	NormalMap,
	SpecularMap,
	ShadowMap,
	ProjectorTexture,
	EnvironmentCubemap,
	Unknown,
	Count
};

enum class ShaderBufferName : uint32_t 
{
	ObjectParams,
	Camera,
	SkinningMatrices,
	Light,
	Projector,
	LightIndices, // SSBO
	LightGrid, // SSBO
	Unknown,
	Count
};



extern const std::map<std::string, ShaderTextureName> SHADER_SAMPLER_NAMES;
extern const std::map<std::string, ShaderBufferName> SHADER_BUFFER_NAMES;
