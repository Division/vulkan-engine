#include "ShaderResource.h"
#include <map>

const std::map<std::string, ShaderResourceName> SHADER_SAMPLER_NAMES = {
	{ "texture0", ShaderResourceName::Texture0 },
	{ "texture1", ShaderResourceName::Texture1 },
	{ "normal_map", ShaderResourceName::NormalMap },
	{ "light_grid", ShaderResourceName::LightGrid },
	{ "light_indices", ShaderResourceName::LightIndices },
	{ "specular_map", ShaderResourceName::SpecularMap },
	{ "shadow_map", ShaderResourceName::ShadowMap },
	{ "projector_texture", ShaderResourceName::ProjectorTexture },
};
