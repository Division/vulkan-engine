#pragma once

#include "CommonIncludes.h"
#include "render/mesh/Mesh.h"

namespace Device { namespace ShaderBufferStruct {

	struct EnvironmentSettings
	{
		mat4 direction_light_projection_matrix;
		float3 direction_light_color;
		uint32_t direction_light_enabled;
		float3 direction_light_direction;
		uint32_t direction_light_cast_shadows;
		float exposure;
		float environment_brightness;
		vec2 padding;
	};

	struct SkinningMatrices 
	{
		mat4 matrices[Mesh::JOINTS_MAX];
	};

	struct Light 
	{
		vec3 position; // offset = 0, size = 12
		float attenuation = 1; // offset = 12, alignment = 4, size = 4
		vec3 color; // offset = 16, alignment = 16, size = 12
		float radius;
		vec3 direction;
		float coneAngle = 0;
		mat4 projectionMatrix;
		vec2 shadowmapScale;
		vec2 shadowmapOffset;
		unsigned int mask;
		vec3 padding;
	};

	struct Projector 
	{
		vec3 position; // offset = 0, size = 12
		float attenuation = 0; // offset = 12, alignment = 4, size = 4
		vec4 color; // offset = 16, alignment = 16, size = 12
		vec2 scale;
		vec2 offset;
		vec2 shadowmapScale;
		vec2 shadowmapOffset;
		mat4 projectionMatrix;
		float radius = 0;
		unsigned int mask;
		vec2 padding; // ending padding
	};

	struct Camera 
	{
		vec3 position; // offset = 0, size = 12
		float zMin;
		uvec2 screenSize = {0,0};
		float zMax;
		float padding = 0;
		mat4 viewMatrix;
		mat4 projectionMatrix;
	};

} }