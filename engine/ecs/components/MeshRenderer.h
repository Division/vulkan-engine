#pragma once

#include "render/renderer/IRenderer.h"
#include "render/shader/ShaderBufferStruct.h"

class Mesh;
class Material;

namespace core { namespace ECS { namespace components {

#pragma pack(push)
#pragma pack(1)
	struct MeshRenderer
	{
		Mesh* mesh = nullptr;
		uint32_t material_id = 0;
		RenderQueue render_queue = RenderQueue::Opaque;
		Device::ShaderBufferStruct::ObjectParams object_params;
		bool draw_call_spawned = false; // true if corresponding draw call object is spawned
	};
#pragma pack(pop)

} } }