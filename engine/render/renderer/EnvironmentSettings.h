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
		float bloom_threshold = 1;
		float bloom_strength = 0.14;
		float bloom_sigma = 4;
		bool bloom_enabled = true;
	};
}