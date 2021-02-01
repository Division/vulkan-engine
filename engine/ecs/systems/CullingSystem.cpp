#include "CullingSystem.h"
#include "ecs/components/DrawCall.h"
#include "ecs/components/Transform.h"
#include "render/renderer/ICameraParamsProvider.h"

namespace ECS { namespace systems {

	void CullingSystem::Process(Chunk* chunk)
	{
		ComponentFetcher<components::DrawCall> draw_call_fetcher(*chunk);

		auto frustum = camera.GetFrustum();

		for (int i = 0; i < chunk->GetEntityCount(); i++)
		{
			auto* draw_call = draw_call_fetcher.GetComponent(i);
			auto& obb = draw_call->obb;
			bool visible = frustum.isVisible(obb.matrix, obb.min, obb.max);

			if (!visible)
				continue;

			draw_call_list->queues[(int)draw_call->queue].push_back(draw_call);
			if (draw_call->queue == RenderQueue::Opaque || draw_call->queue == RenderQueue::AlphaTest)
				draw_call_list->queues[(int)RenderQueue::DepthOnly].push_back(draw_call);
		}
	}

} }