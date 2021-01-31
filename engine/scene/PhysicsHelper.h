#pragma once

#include "Physics.h"
#include "ecs/ECS.h"

namespace Physics::Helper
{
	struct PhysicsInitializer
	{
		enum class Shape
		{
			Sphere,
			Box,
			Plane
		};

		PhysicsInitializer(vec3 position = vec3(), quat rotation = quat(), Shape shape = Shape::Sphere, float size = 1, bool is_static = false, uint32_t word0 = ~0u, uint32_t word1 = ~0u)
			: position(position), rotation(rotation), shape(shape), size(size), is_static(is_static), word0(word0), word1(word1)
		{}


		uint32_t word0;
		uint32_t word1;
		bool is_static;
		vec3 position;
		quat rotation;
		float size;
		Shape shape;// radius or half_size
		physx::PxMaterial* material = nullptr;
	};

	physx::PxRigidActor* AddPhysics(ECS::EntityManager& manager, ECS::EntityID entity, const PhysicsInitializer& init);
}