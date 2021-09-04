#pragma once

#include "CommonIncludes.h"
#include "render/renderer/IRenderer.h"
#include "render/shader/ShaderBufferStruct.h"
#include "render/shader/ShaderBindings.h"
#include "render/material/Material.h"
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
		mat4 transform;
		Device::ShaderProgram* shader = nullptr;
		Device::DescriptorSet* descriptor_set = nullptr;
		Device::ShaderProgram* depth_only_shader = nullptr;
		Device::DescriptorSet* depth_only_descriptor_set = nullptr;
		Device::ConstantBindings constants;
		const Material* material = nullptr;
		uint32_t visible = 0;
		RenderQueue queue = RenderQueue::Opaque;
	};

	struct SkinningData
	{
		Device::ShaderBufferStruct::SkinningMatrices bone_matrices;
	};

}