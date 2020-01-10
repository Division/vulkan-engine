#pragma once

#include "CommonIncludes.h"
#include "Entity.h"

namespace core { namespace ECS { namespace components {

	struct Transform
	{
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