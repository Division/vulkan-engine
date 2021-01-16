#pragma once

#include "render/renderer/IRenderer.h"
#include "render/shader/ShaderBufferStruct.h"
#include "resources/MultiMesh.h"
#include "render/material/Material.h"
#include "utils/DataStructures.h"

class Mesh;
class Material;

namespace ECS::components 
{

	struct MultiMeshRenderer
	{
		Resources::MultiMesh::Handle multi_mesh;
		render::MaterialList::Handle materials;
		RenderQueue render_queue = RenderQueue::Opaque;
		utils::SmallVector<Device::ShaderBufferStruct::ObjectParams, 4> object_params;
		render::DrawCallManager::Handle draw_calls;
	};

}