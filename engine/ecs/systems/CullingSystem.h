#pragma once

#include "ecs/System.h"
#include "render/renderer/DrawCallManager.h"
#include "utils/Frustum.h"

namespace ECS { namespace systems {

	class CullingSystem : public System
	{
	public:
		CullingSystem(render::DrawCallManager& draw_call_manager, Frustum frustum)
			: System(*draw_call_manager.GetManager(), false)
			, draw_call_manager(draw_call_manager)
			, draw_call_list(draw_call_manager.ObtaintDrawCallList())
			, frustum(frustum)
		{}

		render::DrawCallList* GetDrawCallList() const { return draw_call_list; }

		void Process(Chunk* chunk) override;

	private:
		render::DrawCallManager& draw_call_manager;
		Frustum frustum;
		render::DrawCallList* draw_call_list;
	};

} }