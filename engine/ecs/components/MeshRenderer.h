#pragma once

#include "render/renderer/IRenderer.h"
#include "render/shader/ShaderBufferStruct.h"
#include "render/renderer/DrawCallManager.h"
#include "Handle.h"

class Mesh;
class Material;

namespace ECS { namespace components {

	struct MeshRenderer
	{
		Mesh* mesh = nullptr;
		Common::Handle<Material> material;
		RenderQueue render_queue = RenderQueue::Opaque;
		Device::ShaderBufferStruct::ObjectParams object_params;
		render::DrawCallManager::Handle draw_call_handle;
	};

} }