#pragma once

#include "CommonIncludes.h"
#include "utils/CapsSet.h"

enum class ShaderCaps : uint32_t {
	Color = 0,
	ObjectData,
	VertexColor,
	PointSize,
	Billboard,
	Lighting,
	Texture0,
	Texture1,
	NormalMap,
	SpecularMap,
	ProjectedTexture,
	Skinning,
	Count
};

typedef CapsSet<ShaderCaps> ShaderCapsSet;
