#pragma once

#include "render/renderer/IRenderer.h"
#include "render/shader/ShaderBufferStruct.h"
#include "utils/Frustum.h"

namespace ECS::components 
{
	class Transform;

	struct LightBase
	{
		Frustum frustum;
		mat4 view_matrix;
		mat4 projection_matrix;
		mat4 view_projection_matrix;
		vec4 viewport;
		vec3 color = vec3(1, 1, 1);
		float attenuation = 2;
		vec2 shadowmap_scale;
		vec2 shadowmap_offset;
		unsigned int mask = ~0;
		bool cast_shadows = false;
	};

	struct Light : public LightBase
	{
		enum class Type : int 
		{
			Point = 0,
			Spot
		};

		float radius = 5;
		vec3 direction;
		float zMin = 0.05f;
		float cone_angle = 0;
		Type type = Type::Point;

		void UpdateMatrices(Transform& transform);
		Device::ShaderBufferStruct::Light GetShaderStruct(vec3 world_position, float shadow_atlas_size = 1.0f) const;
	};

	struct Projector : public LightBase
	{
		enum class Type : int 
		{
			Decal = 0, // substitutes diffuse color and in some cases normals of the rendered object
			Projector = 1 // Calculated as a light which color is based on the texture
		};

		vec2 scale;
		vec2 offset;
		
		float aspect = 1;
		float zNear = 1;
		float zFar = 10;
		float fov = 30; // Perspective projectors properties in degrees
		Rect sprite_bounds = Rect(0, 0, 1, 1);
		Type type = Type::Decal;
		bool is_orthographic = true;
		float orthographic_size = 1.0f;

		Device::ShaderBufferStruct::Projector GetShaderStruct(vec3 world_position, float shadow_atlas_size = 1.0f) const;
		mat4 GetProjection() const;
	};

}