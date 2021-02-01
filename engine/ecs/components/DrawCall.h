#pragma once

#include "CommonIncludes.h"
#include "render/renderer/IRenderer.h"
#include "render/shader/ShaderBufferStruct.h"
#include "utils/Math.h"

namespace Device
{
	class ShaderProgram;
}

namespace ECS { namespace components {

	class Transform;

	struct DrawCall
	{
		// TODO: remove mesh from draw call
		const Mesh* mesh = nullptr;
		OBB obb;
		Device::ShaderBufferStruct::ObjectParams object_params;
		Device::ShaderProgram* shader = nullptr;
		vk::DescriptorSet descriptor_set;
		Device::ShaderProgram* depth_only_shader = nullptr;
		vk::DescriptorSet depth_only_descriptor_set;
		uint32_t dynamic_offset = 0;
		uint32_t visible = 0;
		RenderQueue queue = RenderQueue::Opaque;
	};

} }