#pragma once

#include "CommonIncludes.h"
#include "Entity.h"
#include "utils/Math.h"
#include "ecs/EntityTemplate.h"

namespace ECS::components 
{

	struct Transform
	{
		AABB bounds = AABB(vec3(-1), vec3(1));
		quat rotation;
		vec3 position = vec3(0,0,0);
		vec3 scale = vec3(1,1,1);
		mat4 local_to_world;
		
		void Rotate(vec3 axis, float angle) 
		{
			rotation = glm::rotate(rotation, angle, axis);
		}

		void Rotate(vec3 euler_angles) 
		{
			rotation = glm::rotate(rotation, euler_angles);
		}

		void Translate(vec3 translation) 
		{
			position += translation;
		}

		void SetMatrix(const mat4 &matrix) 
		{
			quat r;
			vec3 skew;
			vec4 perspective;
			glm::decompose(matrix, scale, r, position, skew, perspective);
			rotation = glm::conjugate(r);
		}

		void LookAt(const vec3& center, const vec3& up = vec3(0, 1, 0))
		{
			SetMatrix(glm::lookAt(position, center, up));
		}

		OBB GetOBB() const { return OBB(local_to_world, bounds.min, bounds.max); }

		vec3 WorldPosition() const { return vec3(local_to_world[3]); }
		const vec3 Left() const  { return -Right(); }
		const vec3 Up() const  { return vec3(local_to_world[1]); }
		const vec3 Forward() const { return -Backward(); }
		const vec3 Right() const { return vec3(local_to_world[0]); }
		const vec3 Down() const { return -Up(); }
		const vec3 Backward() const { return vec3(local_to_world[2]); }

	};

	// Added to any entity that is top level in hierarchy (has no parent)
	struct RootTransform
	{
		EntityID id;
	};

	// Added to any entity that is a not a RootTransform (has parent)
	struct ChildTransform
	{
		EntityID parent_id;
	};


}