#pragma once 

#include "ecs/System.h"
#include "ecs/TransformGraph.h"

namespace ECS { namespace systems {

	class TransformSystem : public System
	{
	public:
		TransformSystem(const TransformGraph& transform_graph, EntityManager& manager) : System(manager), transform_graph(transform_graph) {}

	protected:
		const TransformGraph& transform_graph;
	};

	class RootTransformSystem : public TransformSystem
	{
	public:
		RootTransformSystem(const TransformGraph& transform_graph, EntityManager& manager) : TransformSystem(transform_graph, manager) {}

		void Process(Chunk* chunk) override;
	
	private:
		void SetTransformRecursive(const mat4& matrix, const std::vector<EntityID>& children);
	};

	class NoChildTransformSystem : public TransformSystem
	{
	public:
		NoChildTransformSystem(const TransformGraph& transform_graph, EntityManager& manager) : TransformSystem(transform_graph, manager) {}

		void Process(Chunk* chunk) override;
	};

} }