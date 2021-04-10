#pragma once

#include "render/renderer/IRenderer.h"
#include "render/shading/IShadowCaster.h"
#include "render/renderer/ICameraParamsProvider.h"
#include "render/shader/ShaderBufferStruct.h"
#include "utils/Frustum.h"
#include "Transform.h"

namespace ECS::components 
{
	struct LightBase : public ICameraParamsProvider
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

		virtual vec3 cameraPosition() const override { return view_matrix[3]; };
		virtual mat4 cameraViewProjectionMatrix() const override { return view_projection_matrix; };
		virtual mat4 cameraViewMatrix() const override { return view_matrix; };
		virtual mat4 cameraProjectionMatrix() const override { return projection_matrix; };
		virtual const Frustum& GetFrustum() const override { return frustum; };
		virtual unsigned int cameraVisibilityMask() const override { return mask; };
	};

	struct DirectionalLight : public LightBase
	{
		float zNear = 0.1;
		float zFar = 1000;
		vec2 orthographic_size = vec2(10, 10);
		Transform transform; // it has it's own transform since used as singleton component
		bool enabled = false;
		bool cast_shadows = true;

		virtual vec2 cameraZMinMax() const override { return vec2(zNear, zFar); };
		void UpdateMatrices();
		Device::ShaderBufferStruct::Light GetShaderStruct() const;
	};

	struct Light : public LightBase
	{
		enum class Type : int 
		{
			Point = 0,
			Spot
		};

		float radius = 5;
		float zMin = 0.05f;
		float cone_angle = 45;
		Type type = Type::Point;

		void UpdateMatrices(Transform& transform);
		Device::ShaderBufferStruct::Light GetShaderStruct(vec3 world_position, vec3 direction, float shadow_atlas_size = 1.0f) const;
		vec2 cameraZMinMax() const override { return vec2(zMin, radius); };
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

		virtual vec2 cameraZMinMax() const override { return vec2(zNear, zFar); };
		void UpdateMatrices(Transform& transform);

		Device::ShaderBufferStruct::Projector GetShaderStruct(vec3 world_position, float shadow_atlas_size = 1.0f) const;
		mat4 GetProjection() const;
	};

}