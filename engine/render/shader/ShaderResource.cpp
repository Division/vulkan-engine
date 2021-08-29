#include "ShaderResource.h"
#include <map>
#include <array>
#include "Shader.h"
#include "lib/magic_enum/magic_enum.hpp"

const std::map<std::string, ShaderTextureName> SHADER_TEXTURE_NAMES = 
{
	{ "texture0", ShaderTextureName::Texture0 },
	{ "texture1", ShaderTextureName::Texture1 },
	{ "normal_map", ShaderTextureName::NormalMap },
	{ "specular_map", ShaderTextureName::SpecularMap },
	{ "shadow_map", ShaderTextureName::ShadowMap },
	{ "shadow_map_atlas", ShaderTextureName::ShadowMapAtlas },
	{ "projector_texture", ShaderTextureName::ProjectorTexture },
	{ "environment_cubemap", ShaderTextureName::EnvironmentCubemap },
	{ "radiance_cubemap", ShaderTextureName::RadianceCubemap },
	{ "irradiance_cubemap", ShaderTextureName::IrradianceCubemap },
	{ "brdf_lut", ShaderTextureName::BrdfLUT },
};

const std::map<std::string, ShaderBufferName> SHADER_BUFFER_NAMES = 
{
	{ "ObjectParams", ShaderBufferName::ObjectParams },
	{ "Camera", ShaderBufferName::Camera },
	{ "EnvironmentSettings", ShaderBufferName::EnvironmentSettings },
	{ "SkinningMatrices", ShaderBufferName::SkinningMatrices },
	{ "Lights", ShaderBufferName::Light },
	{ "Projectors", ShaderBufferName::Projector },
	{ "LightGrid", ShaderBufferName::LightGrid },
	{ "LightIndices", ShaderBufferName::LightIndices }
};

const std::map<std::string, ShaderSamplerName> SHADER_SAMPLER_NAMES = 
{
	{ "SamplerLinearWrap", ShaderSamplerName::LinearWrap},
	{ "SamplerLinearClamp", ShaderSamplerName::LinearClamp},
	{ "SamplerPointWrap", ShaderSamplerName::PointWrap},
	{ "SamplerPointClamp", ShaderSamplerName::PointClamp},
};

const std::set<ShaderBufferName> SHADER_DYNAMIC_OFFSET_BUFFERS =
{
	ShaderBufferName::ObjectParams,
	ShaderBufferName::Camera,
	ShaderBufferName::SkinningMatrices
};

namespace Device
{

	namespace
	{
		template<typename T>
		struct NameMap
		{
			std::array<uint32_t, magic_enum::enum_count<T>()> hashes;

			NameMap(const std::map<std::string, T>& name_map)
			{
				hashes.fill(0);
				for (auto& pair : name_map)
					hashes[(size_t)pair.second] = Device::ShaderProgram::GetParameterNameHash(pair.first);
			}
		};
	}

	uint32_t GetShaderTextureNameHash(ShaderTextureName name)
	{
		static NameMap<ShaderTextureName> map(SHADER_TEXTURE_NAMES);
		return map.hashes.at((size_t)name);
	}

	uint32_t GetShaderBufferNameHash(ShaderBufferName name)
	{
		static NameMap<ShaderBufferName> map(SHADER_BUFFER_NAMES);
		return map.hashes.at((size_t)name);
	}

	uint32_t GetShaderBufferHasDynamicOffset(uint32_t name_hash)
	{
		for (auto name : SHADER_DYNAMIC_OFFSET_BUFFERS)
			if (GetShaderBufferNameHash(name) == name_hash)
				return true;

		return false;
	}
}

