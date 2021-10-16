#include "Light.h"
#include "CommonIncludes.h"

namespace ECS::components 
{
	void DirectionalLight::UpdateMatrices()
	{
		const float half_width = orthographic_size.x / 2.0f;
		const float half_height = orthographic_size.y / 2.0f;

		transform.SetLocalToWorld(ComposeMatrix(transform.position, transform.rotation, vec3(1, 1, 1)));

		projection_matrix = glm::ortho(-half_width, half_width, -half_height, half_height, zNear, zFar);
		view_matrix = glm::inverse(transform.GetLocalToWorld());
		view_projection_matrix = projection_matrix * view_matrix;
		frustum.calcPlanes(view_projection_matrix);
	}

	Device::ShaderBufferStruct::Light DirectionalLight::GetShaderStruct() const
	{
		Device::ShaderBufferStruct::Light result;

		result.direction = glm::normalize(transform.Forward());
		result.color = color;
		result.mask = mask;
		result.projectionMatrix = view_projection_matrix;

		return result;
	}

	Device::ShaderBufferStruct::Light Light::GetShaderStruct(vec3 world_position, vec3 direction, float shadow_atlas_size) const
	{
		Device::ShaderBufferStruct::Light result;

		result.position = world_position;
		result.attenuation = attenuation;
		result.radius = radius;
		result.color = color;
		result.mask = mask;

		// Assign for point light as well because point light shadow is calculated like spotlight shadow
		result.coneAngle = cosf(RAD(cone_angle) / 2.0f);
		result.direction = glm::normalize(direction);

		if (cast_shadows) 
		{
			result.shadowmapScale  = vec2(viewport.z, viewport.w) / shadow_atlas_size;
			result.shadowmapOffset = vec2(viewport.x, viewport.y) / shadow_atlas_size;
			result.projectionMatrix = view_projection_matrix;
		} else 
		{
			result.shadowmapScale = vec2(0, 0);
		}

		return result;
	}

	void Light::UpdateMatrices(Transform& transform)
	{
		// Shadow maps are square, so aspect is 1
		projection_matrix = glm::perspective(glm::radians(cone_angle), 1.0f, zMin, radius);
		view_matrix = glm::inverse(transform.GetLocalToWorld());
		view_projection_matrix = projection_matrix * view_matrix;
		frustum.calcPlanes(view_projection_matrix);
	}

	Device::ShaderBufferStruct::Projector Projector::GetShaderStruct(vec3 world_position, float shadow_atlas_size) const 
	{
		Device::ShaderBufferStruct::Projector result;

		result.position = world_position;
		result.attenuation = attenuation;
		result.radius = zFar;
		result.color = vec4(color, 1);
		result.scale = vec2(sprite_bounds.width, sprite_bounds.height);
		result.offset = vec2(sprite_bounds.x, sprite_bounds.y);
		result.mask = mask;

		if (cast_shadows) 
		{
			result.shadowmapScale = vec2(viewport.z, viewport.w) / shadow_atlas_size;
			result.shadowmapOffset= vec2(viewport.x, viewport.y) / shadow_atlas_size;
		} else 
		{
			result.shadowmapScale = vec2(0, 0);
			result.shadowmapOffset = vec2(0, 0);
		}

		result.projectionMatrix = view_projection_matrix;

		return result;
	}

	void Projector::UpdateMatrices(Transform& transform)
	{
		projection_matrix = GetProjection();
		view_matrix = glm::inverse(transform.GetLocalToWorld());
		view_projection_matrix = projection_matrix * view_matrix;
		frustum.calcPlanes(view_projection_matrix);
	}

	mat4 Projector::GetProjection() const 
	{
		if (is_orthographic) 
		{
			float halfHeight = orthographic_size / 2.0f;
			float halfWidth = aspect * halfHeight;
			return glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, zNear, zFar);
		} else 
		{
			return glm::perspective(RAD(fov), aspect, zNear, zFar);
		}
	}

}