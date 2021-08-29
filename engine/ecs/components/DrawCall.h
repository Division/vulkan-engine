#pragma once

#include "CommonIncludes.h"
#include "render/renderer/IRenderer.h"
#include "render/shader/ShaderBufferStruct.h"
#include "utils/Math.h"

namespace Device
{
	class ShaderProgram;
	class DescriptorSet;
}

namespace ECS::components {

	class Transform;

	struct DrawCall
	{
		// TODO: remove mesh from draw call
		const Mesh* mesh = nullptr;
		OBB obb;
		Device::ShaderBufferStruct::ObjectParams object_params;
		Device::ShaderProgram* shader = nullptr;
		Device::DescriptorSet* descriptor_set = nullptr;
		Device::ShaderProgram* depth_only_shader = nullptr;
		Device::DescriptorSet* depth_only_descriptor_set = nullptr;
		uint32_t dynamic_offset = 0;
		uint32_t skinning_dynamic_offset = -1;
		uint32_t visible = 0;
		RenderQueue queue = RenderQueue::Opaque;
	};

	struct SkinningData
	{
		Device::ShaderBufferStruct::SkinningMatrices bone_matrices;
	};

}