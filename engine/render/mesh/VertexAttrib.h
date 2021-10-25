#pragma once

#include "utils/DataStructures.h"
#include "utils/CapsSet.h"
#include "utils/Math.h"
#include "render/device/Types.h"
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
	JointIndices,
	Origin,
	Unknown,
	Count
};

class VertexLayout
{
public:
	struct Attrib
	{
		VertexAttrib attrib;
		Device::Format format;
		uint32_t offset;
	};

	VertexLayout()
	{
		Clear();
	}

	void AddAttrib(VertexAttrib attrib, Device::Format format, uint32_t size)
	{
		attribs[(uint32_t)attrib] = { attrib, format, stride };
		stride += size;
		hash = FastHash(attribs.data(), sizeof(attribs));
	}

	void Clear()
	{
		for (auto& attrib : attribs)
			attrib = { VertexAttrib::Unknown, Device::Format::Undefined , (uint32_t)-1 };

		stride = 0;
		hash = 0;
	}

	bool HasAttrib(VertexAttrib attrib) const
	{
		return attribs[(uint32_t)attrib].offset != -1;
	}

	uint32_t GetHash() const { return hash; }
	uint32_t GetStride() const { return stride; }
	uint32_t GetAttribOffset(VertexAttrib attrib) const
	{
		assert(attribs[(uint32_t)attrib].offset != -1);
		return attribs[(uint32_t)attrib].offset;
	}

	Device::Format GetAttribFormat(VertexAttrib attrib) const
	{
		assert(attribs[(uint32_t)attrib].format != Device::Format::Undefined);
		return attribs[(uint32_t)attrib].format;
	}

private:
	uint32_t stride = 0;
	uint32_t hash = 0;
	std::array<Attrib, (uint32_t)VertexAttrib::Count> attribs;
};



typedef CapsSet<VertexAttrib> VertexAttribSet;

typedef std::shared_ptr<VertexAttribSet> VertexAttribSetPtr;