#pragma once

#include "CommonIncludes.h"
#include "render/mesh/VertexAttrib.h"
#include "ShaderResource.h"
#include "ShaderCaps.h"

namespace core { namespace Device {

	enum DescriptorSet : int
	{
		Global,
		Object
	};

	extern const std::map<std::string, VertexAttrib> VERTEX_ATTRIB_NAMES;
	extern const std::map<ShaderCaps, std::string> SHADER_CAPS_DEFINES;

} }