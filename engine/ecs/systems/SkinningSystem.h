#pragma once

#include "ecs/System.h"

namespace ECS::systems {

	class SkinningSystem : public System
	{
	public:
		SkinningSystem(EntityManager& manager)
			: System(manager, false)
		{}

		void Process(Chunk* chunk) override;

	};

	class BoneAttachmentSystem : public System
	{
	public:
		BoneAttachmentSystem(EntityManager& manager)
			: System(manager, false)
		{}

		void Process(Chunk* chunk) override;
	};

	class DebugDrawSkinningSystem : public System
	{
	public:
		DebugDrawSkinningSystem(EntityManager& manager)
			: System(manager, false)
		{}

		void Process(Chunk* chunk) override;

	};

	class DebugDrawSkinningVerticesSystem : public System
	{
	public:
		DebugDrawSkinningVerticesSystem(EntityManager& manager)
			: System(manager, false)
		{}

		void Process(Chunk* chunk) override;

	};

}