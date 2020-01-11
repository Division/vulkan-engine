#pragma once

#include "render/renderer/IRenderer.h"
#include "render/shader/ShaderBufferStruct.h"

class Mesh;
class Material;

namespace core { namespace ECS { namespace components {

	struct MeshRenderer
	{
		Mesh* mesh = nullptr;
		Material* material = nullptr;
		RenderQueue render_queue = RenderQueue::Opaque;
		Device::ShaderBufferStruct::ObjectParams object_params;
	};

} } }