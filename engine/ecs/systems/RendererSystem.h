#pragma once 

#include <mutex>
#include "ecs/System.h"
#include "render/renderer/IRenderer.h"

namespace ECS { namespace systems {

	// Updates MeshRenderer component with proper data
	class UpdateRendererSystem : public System
	{
	public:
		UpdateRendererSystem(EntityManager& manager) : System(manager) {}

		void Process(Chunk* chunk) override;
	
	private:
		void ProcessMeshRenderer(Chunk* chunk);
		void ProcessMultiMeshRenderer(Chunk* chunk);
	};

} }