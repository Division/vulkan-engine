#pragma once

#include "CommonIncludes.h"
#include "render/mesh/VertexAttrib.h"
#include "ShaderResource.h"
#include "ShaderCaps.h"

namespace core { namespace Device {

	extern const std::map<VertexAttrib, std::string> SHADER_ATTRIB_DEFINES;
	extern const std::map<std::string, VertexAttrib> VERTEX_ATTRIB_NAMES;
	extern const std::map<VertexAttrib, vk::Format> VERTEX_ATTRIB_FORMATS;

	extern const std::map<ShaderResourceName, std::string> SHADER_RESOURCE_DEFINES;

	extern const std::map<ConstantBufferName, std::string> CONSTANT_BUFFER_DEFINES;

	extern const std::map<ShaderCaps, std::string> SHADER_CAPS_DEFINES;

	extern const std::string SHADER_IS_VERTEX;
	extern const std::string SHADER_IS_PIXEL;

} }