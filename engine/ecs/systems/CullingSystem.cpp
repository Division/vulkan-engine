#include "CullingSystem.h"
#include "ecs/components/DrawCall.h"
#include "render/renderer/ICameraParamsProvider.h"

namespace ECS { namespace systems {

	void CullingSystem::Process(Chunk* chunk)
	{
		ComponentFetcher<components::DrawCall> draw_call_fetcher(*chunk);

		for (int i = 0; i < chunk->GetEntityCount(); i++)
		{
			auto* draw_call = draw_call_fetcher.GetComponent(i);
			bool visible = true; // TODO: proper check

			if (!visible)
				continue;

			draw_call_list->queues[(int)draw_call->queue].push_back(draw_call);
			if (draw_call->queue == RenderQueue::Opaque || draw_call->queue == RenderQueue::AlphaTest)
				draw_call_list->queues[(int)RenderQueue::DepthOnly].push_back(draw_call);
		}
	}

} }