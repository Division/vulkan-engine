#pragma once 

#include <mutex>
#include "ecs/System.h"
#include "render/renderer/RenderOperation.h"
#include "render/renderer/IRenderer.h"

namespace core { namespace ECS { namespace systems {

	// Generates list of rops + render queue from the MeshRenderer components
	// TODO: refactor ROPs so that they are not recreated every frame
	class RendererToROPSystem : public System
	{
	public:
		RendererToROPSystem(EntityManager& manager) : System(manager) {}

		void Process(Chunk* chunk) override;

		auto& GetRops() const { return rops; }
		void ResetRops() { rops.clear(); }

	private:
		void AppendRops(const std::vector<std::pair<RenderQueue, RenderOperation>>& chunk_rops);
		std::vector<std::pair<RenderQueue, RenderOperation>> rops;
		std::mutex mutex;
	};

	// Updates MeshRenderer component with proper data
	class UpdateRendererSystem : public System
	{
	public:
		UpdateRendererSystem(EntityManager& manager) : System(manager) {}

		void Process(Chunk* chunk) override;
	
	private:
		void ProcessMeshRenderer(Chunk* chunk);
		void ProcessMeshRendererSkinning(Chunk* chunk);

	};

} } }