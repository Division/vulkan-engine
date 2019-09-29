﻿#include "ShaderDefines.h"

namespace core { namespace Device {

	const std::map<VertexAttrib, std::string> SHADER_ATTRIB_DEFINES = {
		{ VertexAttrib::Position, "ATTRIB_POSITION" },
		{ VertexAttrib::Normal, "ATTRIB_NORMAL" },
		{ VertexAttrib::Tangent, "ATTRIB_TANGENT" },
		{ VertexAttrib::Bitangent, "ATTRIB_BITANGENT" },
		{ VertexAttrib::TexCoord0, "ATTRIB_TEXCOORD0" },
		{ VertexAttrib::Corner, "ATTRIB_CORNER" },
		{ VertexAttrib::VertexColor, "ATTRIB_VERTEX_COLOR" },
		{ VertexAttrib::JointWeights, "ATTRIB_JOINT_WEIGHT" },
		{ VertexAttrib::JointIndices, "ATTRIB_JOINT_INDEX" }
	};

	const std::map<std::string, VertexAttrib> VERTEX_ATTRIB_NAMES =
	{
		{ "position", VertexAttrib::Position },
		{ "normal", VertexAttrib::Normal },
		{ "tangent", VertexAttrib::Tangent },
		{ "bitangent", VertexAttrib::Bitangent },
		{ "texcoord0", VertexAttrib::TexCoord0 },
		{ "corner", VertexAttrib::Corner },
		{ "color", VertexAttrib::VertexColor },
		{ "joint_weights", VertexAttrib::JointWeights },
		{ "joint_indices", VertexAttrib::JointIndices }
	};

	const std::map<VertexAttrib, vk::Format> VERTEX_ATTRIB_FORMATS
	{
		{ VertexAttrib::Position, vk::Format::eR32G32B32Sfloat },
		{ VertexAttrib::Normal, vk::Format::eR32G32B32Sfloat },
		{ VertexAttrib::Tangent, vk::Format::eR32G32B32Sfloat },
		{ VertexAttrib::Bitangent, vk::Format::eR32G32B32Sfloat },
		{ VertexAttrib::TexCoord0, vk::Format::eR32G32Sfloat },
		{ VertexAttrib::Corner, vk::Format::eR32G32Sfloat  },
		{ VertexAttrib::VertexColor, vk::Format::eR32G32B32A32Sfloat },
		{ VertexAttrib::JointWeights, vk::Format::eR32G32B32Sfloat },
		{ VertexAttrib::JointIndices, vk::Format::eR32G32B32Sfloat }
	};

	const std::map<ShaderTextureName, std::string> SHADER_RESOURCE_DEFINES = {
		{ ShaderTextureName::Texture0, "RESOURCE_TEXTURE0" },
		{ ShaderTextureName::Texture1, "RESOURCE_TEXTURE1" },
		{ ShaderTextureName::NormalMap, "RESOURCE_NORMAL_MAP" },
		{ ShaderTextureName::SpecularMap, "RESOURCE_SPECULAR_MAP" },
		{ ShaderTextureName::ShadowMap, "RESOURCE_SHADOW_MAP" },
		{ ShaderTextureName::ProjectorTexture, "RESOURCE_PROJECTOR_TEXTURE" },
		{ ShaderTextureName::LightGrid, "RESOURCE_LIGHT_GRID" },
		{ ShaderTextureName::LightIndices, "RESOURCE_LIGHT_INDICES" }
	};

	const std::map<ShaderBufferName, std::string> CONSTANT_BUFFER_DEFINES = {
		{ ShaderBufferName::ObjectParams, "CONSTANT_BUFFER_OBJECT_PARAMS" },
		{ ShaderBufferName::SkinningMatrices, "CONSTANT_BUFFER_SKINNING_MATRICES" },
		{ ShaderBufferName::Light, "CONSTANT_BUFFER_LIGHT" },
		{ ShaderBufferName::Camera, "CONSTANT_BUFFER_CAMERA" },
		{ ShaderBufferName::Projector, "CONSTANT_BUFFER_PROJECTOR" }
	};

	const std::map<ShaderCaps, std::string> SHADER_CAPS_DEFINES = {
		{ ShaderCaps::Color, "CAP_COLOR" },
		{ ShaderCaps::ObjectData, "CAP_OBJECT_DATA" },
		{ ShaderCaps::VertexColor, "CAP_VERTEX_COLOR" },
		{ ShaderCaps::PointSize, "CAP_POINT_SIZE" },
		{ ShaderCaps::Billboard, "CAP_BILLBOARD" },
		{ ShaderCaps::Lighting, "CAP_LIGHTING" },
		{ ShaderCaps::Texture0, "CAP_TEXTURE0" },
		{ ShaderCaps::Texture1, "CAP_TEXTURE1" },
		{ ShaderCaps::NormalMap, "CAP_NORMAL_MAP" },
		{ ShaderCaps::SpecularMap, "CAP_SPECULAR_MAP" },
		{ ShaderCaps::ProjectedTexture, "CAP_PROJECTED_TEXTURE" },
		{ ShaderCaps::Skinning, "CAP_SKINNING" }
	};

	const std::string SHADER_IS_VERTEX = "IS_VERTEX";
	const std::string SHADER_IS_PIXEL = "IS_PIXEL";

} }