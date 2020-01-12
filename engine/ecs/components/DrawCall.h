#pragma once

#include "CullingData.h"
#include "render/renderer/IRenderer.h"
#include "render/shader/ShaderBufferStruct.h"

namespace core { namespace ECS { namespace components {

	struct DrawCall
	{
		struct CullingData
		{
			CullingData::Type type;
			union
			{
				Sphere sphere;
				AABB bounds;
			};
		} culling_data;

		Device::ShaderBufferStruct::ObjectParams* object_params = nullptr;
		uint32_t visible = 0;
	}

} } }