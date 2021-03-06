#pragma once

namespace ECS::components
{
	struct DirectionalLight;
}

namespace render
{
	struct EnvironmentSettings
	{
		ECS::components::DirectionalLight* directional_light = nullptr;
		float exposure = 1;
		float environment_brightness = 1;
	};
}