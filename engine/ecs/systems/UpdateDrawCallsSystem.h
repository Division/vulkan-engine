#pragma once 

#include <mutex>
#include "ecs/System.h"
#include "render/renderer/IRenderer.h"
#include "ecs/components/DrawCall.h"
#include "render/renderer/DrawCallManager.h"

namespace core
{
	namespace render
	{
		class SceneBuffers;
	}
}

namespace core { namespace ECS { namespace systems {

	// Generates list of rops + render queue from the MeshRenderer components
	class CreateDrawCallsSystem : public System
	{
	public:
		CreateDrawCallsSystem(EntityManager& manager, render::DrawCallManager& draw_call_manager) 
			: System(manager)
			, draw_call_manager(draw_call_manager)
		{}

		void Process(Chunk* chunk) override;
	
	private:
		render::DrawCallManager& draw_call_manager;
	};

	// Uploads draw call data to the GPU buffers
	class UploadDrawCallsSystem : public System
	{
	public:
		UploadDrawCallsSystem(render::DrawCallManager& draw_call_manager, render::SceneBuffers& scene_buffers) 
			: System(*draw_call_manager.GetManager(), false)
			, scene_buffers(scene_buffers)
			, draw_call_manager(draw_call_manager)
		{}

		void Process(Chunk* chunk) override;

	private:
		render::DrawCallManager& draw_call_manager;
		render::SceneBuffers& scene_buffers;
	};

} } }