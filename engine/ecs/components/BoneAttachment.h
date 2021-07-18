
#pragma once

#include "ecs/ECS.h"

namespace ECS::components
{

	// Entity with this component is attached to a joint
	struct BoneAttachment
	{
		ECS::EntityID entity_id;
		uint32_t joint_index;
	};

}