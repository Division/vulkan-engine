#pragma once

#include "CommonIncludes.h"
#include "Entity.h"

namespace core { namespace ECS { namespace components {

	struct Transform
	{
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

		void Transform::SetMatrix(const mat4 &matrix) 
		{
			quat r;
			vec3 skew;
			vec4 perspective;
			glm::decompose(matrix, scale, r, position, skew, perspective);
			rotation = glm::conjugate(r);
		}

		quat rotation;
		vec3 position;
		vec3 scale = vec3(1,1,1);
		mat4 local_to_world;
	};

	struct RootTransform
	{
		EntityID id;
	};

	struct ChildTransform
	{
		EntityID parent_id;
	};

} } }