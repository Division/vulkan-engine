#pragma once

#include "ecs/System.h"
#include "ecs/components/Batching.h"
#include <unordered_map>

namespace ECS::components
{
	struct MultiMeshRenderer;
	struct Transform;
}

namespace ECS::systems 
{

	class BatchingVolumeSystem : public System
	{
		typedef std::unordered_multimap<uint32_t, components::BatchingVolume::BatchSrc*> BatchMap;

		void AppendMesh(BatchMap& batch_map, components::Transform* transform, components::BatchingVolume* volume, components::MultiMeshRenderer* mesh_renderer);

	public:
		BatchingVolumeSystem(EntityManager& manager) : System(manager) {}

		void Process(Chunk* chunk) override;
	};

}