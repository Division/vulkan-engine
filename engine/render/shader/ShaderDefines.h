#pragma once

#include "CommonIncludes.h"
#include "render/mesh/VertexAttrib.h"
#include "ShaderResource.h"
#include "ShaderCaps.h"

namespace core { namespace Device {

	enum class DescriptorSet
	{
		Pass,
		Object,
		//Material // Not used yet
	};

	extern const std::map<std::string, VertexAttrib> VERTEX_ATTRIB_NAMES;
	extern const std::map<VertexAttrib, vk::Format> VERTEX_ATTRIB_FORMATS;

	extern const std::map<ShaderCaps, std::string> SHADER_CAPS_DEFINES;

} }