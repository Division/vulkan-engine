#include "ShaderDefines.h"

namespace Device {

	// It's SEMANTIC name, not the attribute
	const std::map<std::string, VertexAttrib> VERTEX_ATTRIB_NAMES =
	{
		{ "position", VertexAttrib::Position },
		{ "normal", VertexAttrib::Normal },
		{ "tangent", VertexAttrib::Tangent },
		{ "bitangent", VertexAttrib::Bitangent },
		{ "binormal", VertexAttrib::Bitangent },
		{ "texcoord0", VertexAttrib::TexCoord0 },
		{ "texcoord", VertexAttrib::TexCoord0 },
		{ "corner", VertexAttrib::Corner },
		{ "color", VertexAttrib::VertexColor },
		{ "joint_weights", VertexAttrib::JointWeights },
		{ "joint_indices", VertexAttrib::JointIndices },
		{ "blendweight", VertexAttrib::JointWeights },
		{ "blendindices", VertexAttrib::JointIndices },
		{ "position1", VertexAttrib::Origin }
	};

	const std::map<ShaderCaps, std::string> SHADER_CAPS_DEFINES = {
		{ ShaderCaps::Color, "COLOR" },
		{ ShaderCaps::VertexColor, "VERTEX_COLOR" },
		{ ShaderCaps::PointSize, "POINT_SIZE" },
		{ ShaderCaps::Billboard, "BILLBOARD" },
		{ ShaderCaps::Lighting, "LIGHTING" },
		{ ShaderCaps::Texture0, "TEXTURE0" },
		{ ShaderCaps::Texture1, "TEXTURE1" },
		{ ShaderCaps::NormalMap, "NORMAL_MAP" },
		{ ShaderCaps::SpecularMap, "SPECULAR_MAP" },
		{ ShaderCaps::Skinning, "SKINNING" },
		{ ShaderCaps::VertexTBN, "VERTEX_TBN" },
		{ ShaderCaps::VertexOrigin, "VERTEX_ORIGIN" },
		{ ShaderCaps::DepthOnly, "DEPTH_ONLY" },
		{ ShaderCaps::AlphaCutoff, "ALPHA_CUTOFF" }
	};

}