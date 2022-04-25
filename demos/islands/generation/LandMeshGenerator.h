#pragma once

#include <ecs/ECS.h>
#include <string_view>

namespace Generation
{
	ECS::EntityID CreateLandMesh(ECS::EntityManager& manager, vec3 position, std::wstring_view heightmap_path);
}