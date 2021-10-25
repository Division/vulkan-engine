#pragma once

#include "utils/CapsSet.h"

enum class ShaderCaps : uint32_t {
	Color = 0,
	VertexColor,
	PointSize,
	Billboard,
	Lighting,
	Texture0,
	Texture1,
	NormalMap,
	SpecularMap,
	Skinning,
	VertexOrigin,
	VertexTBN,
	DepthOnly,
	AlphaCutoff,
	Count
};

typedef CapsSet<ShaderCaps> ShaderCapsSet;
