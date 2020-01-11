#include "RendererSystem.h"
#include "ecs/components/MeshRenderer.h"
#include "ecs/components/Transform.h"

namespace core { namespace ECS { namespace systems {

	using namespace core::ECS::components;

	void RendererToROPSystem::Process(Chunk* chunk)
	{
		static thread_local std::vector<std::pair<RenderQueue, RenderOperation>> local_rops;
		local_rops.clear();

		ComponentFetcher<MeshRenderer> mesh_renderer_fetcher(*chunk);
		for (int i = 0; i < chunk->GetEntityCount(); i++)
		{
			auto* mesh_renderer = mesh_renderer_fetcher.GetComponent(i);
			RenderOperation rop;
			rop.material = mesh_renderer->material;
			rop.mesh = mesh_renderer->mesh;
			rop.object_params = &mesh_renderer->object_params;

			local_rops.push_back(std::make_pair(mesh_renderer->render_queue, rop));
		}

		AppendRops(local_rops);
	}

	void RendererToROPSystem::AppendRops(const std::vector<std::pair<RenderQueue, RenderOperation>>& chunk_rops)
	{
		std::lock_guard<std::mutex> lock(mutex);
		rops.insert(rops.end(), chunk_rops.begin(), chunk_rops.end());
	}

	void UpdateRendererSystem::Process(Chunk* chunk)
	{
		//auto& layout = chunk->GetComponentLayout();
		ProcessMeshRenderer(chunk);
	}

	void UpdateRendererSystem::ProcessMeshRenderer(Chunk* chunk)
	{
		ComponentFetcher<MeshRenderer> mesh_renderer_fetcher(*chunk);
		ComponentFetcher<Transform> transform_fetcher(*chunk);

		for (int i = 0; i < chunk->GetEntityCount(); i++)
		{
			auto* mesh_renderer = mesh_renderer_fetcher.GetComponent(i);
			auto* transform = transform_fetcher.GetComponent(i);
			mesh_renderer->object_params.transform = transform->local_to_world;
			mesh_renderer->object_params.uvOffset = vec2(0, 0);
			mesh_renderer->object_params.uvScale = vec2(1, 1);
		}
	}

	void UpdateRendererSystem::ProcessMeshRendererSkinning(Chunk* chunk)
	{
		// TODO: implement later
	}

} } }