#pragma once

#include "utils/DataStructures.h"
#include "utils/CapsSet.h"
#include "utils/Math.h"
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
	Unknown,
	Count
};

namespace core {

	class VertexLayout
	{
	public:
		VertexLayout()
		{
			Clear();
		}

		void AddAttrib(VertexAttrib attrib, uint32_t size)
		{
			attribs[(uint32_t)attrib] = std::make_pair(attrib, stride);
			stride += size;
			hash = FastHash(attribs.data(), sizeof(attribs));
		}

		void Clear()
		{
			for (auto& attrib : attribs)
				attrib = std::make_pair(VertexAttrib::Unknown, -1);

			stride = 0;
			hash = 0;
		}

		uint32_t GetHash() const { return hash; }
		uint32_t GetStride() const { return stride; }
		uint32_t GetAttribOffset(VertexAttrib attrib) const
		{
			assert(attribs[(uint32_t)attrib].second != -1);
			return attribs[(uint32_t)attrib].second;
		}

	private:
		uint32_t stride = 0;
		uint32_t hash = 0;
		std::array<std::pair<VertexAttrib, uint32_t>, (uint32_t)VertexAttrib::Count> attribs;
	};

}

typedef CapsSet<VertexAttrib> VertexAttribSet;

typedef std::shared_ptr<VertexAttribSet> VertexAttribSetPtr;