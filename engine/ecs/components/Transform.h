#pragma once

#include "Entity.h"

namespace core { namespace ECS { namespace components {

	struct Transform
	{

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