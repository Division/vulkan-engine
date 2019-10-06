#include "ShaderResource.h"
#include <map>

const std::map<std::string, ShaderTextureName> SHADER_SAMPLER_NAMES = {
	{ "texture0", ShaderTextureName::Texture0 },
	{ "texture1", ShaderTextureName::Texture1 },
	{ "normal_map", ShaderTextureName::NormalMap },
	{ "light_grid", ShaderTextureName::LightGrid },
	{ "light_indices", ShaderTextureName::LightIndices },
	{ "specular_map", ShaderTextureName::SpecularMap },
	{ "shadow_map", ShaderTextureName::ShadowMap },
	{ "projector_texture", ShaderTextureName::ProjectorTexture },
};

const std::map<std::string, ShaderBufferName> SHADER_BUFFER_NAMES = {
	{ "ObjectParams", ShaderBufferName::ObjectParams },
	{ "Camera", ShaderBufferName::Camera },
	{ "SkinningMatrices", ShaderBufferName::SkinningMatrices },
	{ "Lights", ShaderBufferName::Light },
	{ "Projectors", ShaderBufferName::Projector },
};
