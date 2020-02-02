#include "ShaderDefines.h"

namespace core { namespace Device {

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
		{ ShaderCaps::Skinning, "SKINNING" }
	};

} }