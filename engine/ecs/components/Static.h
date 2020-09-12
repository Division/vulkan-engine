#pragma once

namespace ECS { namespace components {

	struct DeltaTime
	{
		float dt = 0;
		float physics_dt = 0;
	};

} }