#pragma once

#include "CommonIncludes.h"
#include "render/renderer/IRenderer.h"
#include "render/shader/ShaderBufferStruct.h"

namespace Device
{
	class ShaderProgram;
}

namespace ECS { namespace components {

	class Transform;

#pragma pack(push)
#pragma pack(1)
	struct DrawCall
	{
		// TODO: remove mesh from draw call
		const Mesh* mesh = nullptr;
		Transform* transform;
		Device::ShaderBufferStruct::ObjectParams* object_params;
		Device::ShaderProgram* shader;
		vk::DescriptorSet descriptor_set;
		Device::ShaderProgram* depth_only_shader;
		vk::DescriptorSet depth_only_descriptor_set;
		uint32_t dynamic_offset = 0;
		uint32_t visible = 0;
		RenderQueue queue = RenderQueue::Opaque;
	};
#pragma pack(pop)

} }