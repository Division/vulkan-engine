#pragma once

namespace render
{
	struct EnvironmentSettings
	{
		vec4 direction_light_color = vec4(1, 1, 1, 0);
		vec4 direction_light_direction = vec4(1,-1,1, 0);
		float exposure = 1;
		float environment_brightness = 1;
	};
}