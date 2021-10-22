#pragma once

#include "ecs/System.h"
#include "ecs/components/Batching.h"
#include <unordered_map>
#include "system/JobSystem.h"

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
		class ProcessBatchingJob;

	public:
		BatchingVolumeSystem(EntityManager& manager) : System(manager) {}

		void Process(Chunk* chunk) override;
	};

}