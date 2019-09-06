#pragma once

#include "utils/CapsSet.h"
#include <memory>

enum class VertexAttrib : uint32_t {
	Position = 0,
	Normal,
	Bitangent,
	Tangent,
	TexCoord0,
	Corner,
	VertexColor,
	TexCoord1,
	JointWeights,
	JointIndices
};

typedef CapsSet<VertexAttrib> VertexAttribSet;

typedef std::shared_ptr<VertexAttribSet> VertexAttribSetPtr;