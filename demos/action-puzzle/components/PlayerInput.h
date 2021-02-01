#pragma once

namespace ECS::components
{

	struct PlayerInput
	{
		bool accelerate = false;
		bool brake = false;
		bool back = false;
		bool turn_left = false;
		bool turn_right = false;
	};

}