#pragma once

#include "render/renderer/IRenderer.h"
#include "render/shader/ShaderBufferStruct.h"

namespace core { namespace ECS { namespace components {

	struct CullingData
	{
		enum class Type : int 
		{
			Sphere,
			OBB
		};

		struct SphereData
		{
			Sphere local;
			Sphere worldspace;
		};

		CullingData::Type type;
		union
		{
			SphereData sphere;
			AABB bounds; // this aabb is transformed by Transform::local_to_world matrix to get OBB
		};
	};

} } }