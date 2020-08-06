#pragma once

#include "CommonIncludes.h"
#include "render/mesh/Mesh.h"

namespace Device { namespace ShaderBufferStruct {

	struct ObjectParams 
	{
		mat4 transform; // offset = 0, size = 64
		mat4 normalMatrix; // offset = 64, alignment = 16, size = 64
		vec2 uvScale = vec2(1, 1);
		vec2 uvOffset = vec2(0, 0);
		unsigned int layer = 1u << 0;
		float roughness = 0.0f;
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
		float padding1 = 0;
		uvec2 screenSize = {0,0};
		vec2 padding23 = {0,0};
		mat4 viewMatrix;
		mat4 projectionMatrix;
	};

} }