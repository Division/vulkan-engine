#pragma once

#include "CommonIncludes.h"
#include "CullingData.h"
#include "render/renderer/IRenderer.h"
#include "render/shader/ShaderBufferStruct.h"

namespace core
{
	namespace Device
	{
		class ShaderProgram;
	}
}

namespace core { namespace ECS { namespace components {

#pragma pack(push)
#pragma pack(1)
	struct DrawCall
	{
		/*struct CullingData
		{
			CullingData::Type type;
			union
			{
				Sphere sphere;
				AABB bounds;
			};
		} culling_data;*/

		const Mesh* mesh = nullptr;
		ShaderBufferStruct::ObjectParams* object_params;
		Device::ShaderProgram* shader;
		vk::DescriptorSet descriptor_set;
		Device::ShaderProgram* depth_only_shader;
		vk::DescriptorSet depth_only_descriptor_set;
		uint32_t dynamic_offset = 0;
		uint32_t visible = 0;
		RenderQueue queue = RenderQueue::Opaque;
	};
#pragma pack(pop)

} } }