#pragma once

#include "CommonIncludes"
#include "CullingData.h"
#include "render/renderer/IRenderer.h"
#include "render/shader/ShaderBufferStruct.h"

namespace core { namespace ECS { namespace components {

#pragma pack push
#pragma pack 1
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

		const Mesh* mesh = nullptr;
		Device::Shader* shader;
		vk::DescriptorSet object_descriptor_set;
		uint32_t visible = 0;
	}
#pragma pack pop

} } }