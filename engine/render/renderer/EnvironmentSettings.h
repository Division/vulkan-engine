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
		float bloom_threshold = 0.42;
		float bloom_strength = 0.14;
		float bloom_scatter = 0.6305;
		float bloom_clamp_max = 25000;
		bool bloom_enabled = true;
	};
}