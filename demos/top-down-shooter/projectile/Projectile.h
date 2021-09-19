#pragma once

#include <glm/vec3.hpp>
#include <render/material/Material.h>
#include <render/mesh/Mesh.h>
#include <ecs/ECS.h>
#include <ecs/EntityChunks.h>
#include <ecs/System.h>
#include <ecs/CommandBuffer.h>

using namespace glm;

namespace ECS::components
{

	struct Projectile
	{
		vec3 direction;
		vec3 prev_position;
		float speed = 1.0f;
		float distance_travelled = 0.0f;
		float max_distance = 10.0f;
	};

}

namespace ECS::systems
{
	class ProjectileSystem : public ECS::System
	{
		CommandBuffer command_buffer;

	public:
		ProjectileSystem(ECS::EntityManager& manager)
			: ECS::System(manager, false)
			, command_buffer(manager)
		{}

		void Process(ECS::Chunk* chunk) override;

		CommandBuffer& GetCommandBuffer() { return command_buffer; }
	};

}

namespace Projectile
{

	struct Params
	{
		vec3 position = vec3(0, 0, 0);
		vec3 direction = vec3(1, 0, 0);
		vec2 dimensions = vec2(0.3, 0.3);
		Common::Handle<Mesh> mesh;
		float speed = 1.0f;
		float max_distance = 10.0f;
		Material::Handle material;
	};

	class ProjectileManager
	{
		ECS::EntityManager& manager;
		Common::Handle<Mesh> mesh;
		Material::Handle material;
		ECS::systems::ProjectileSystem projectile_system;

	public:
		ProjectileManager(ECS::EntityManager& manager);
	
		ECS::EntityID CreateProjectile(const Params& params);
		void Update();
	};

}
