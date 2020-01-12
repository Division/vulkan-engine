#pragma once

#include "render/renderer/IRenderer.h"
#include "render/shader/ShaderBufferStruct.h"

class Mesh;
class Material;

namespace core { namespace ECS { namespace components {

	struct MeshRenderer
	{
		Mesh* mesh = nullptr;
		uint32_t material_id = 0;
		RenderQueue render_queue = RenderQueue::Opaque;
		Device::ShaderBufferStruct::ObjectParams object_params;
	};

} } }