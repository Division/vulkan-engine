#pragma once

#include "ecs/System.h"
#include "render/renderer/DrawCallManager.h"

class ICameraParamsProvider;

namespace core { namespace ECS { namespace systems {

	// Uploads draw call data to the GPU buffers
	class CullingSystem : public System
	{
	public:
		CullingSystem(render::DrawCallManager& draw_call_manager, ICameraParamsProvider& camera) 
			: System(*draw_call_manager.GetManager(), false)
			, draw_call_manager(draw_call_manager)
			, draw_call_list(draw_call_manager.ObtaintDrawCallList())
			, camera(camera)
		{}

		render::DrawCallList* GetDrawCallList() const { return draw_call_list; }

		void Process(Chunk* chunk) override;

	private:
		render::DrawCallManager& draw_call_manager;
		ICameraParamsProvider& camera;
		render::DrawCallList* draw_call_list;
	};

} } }