#include "Projectile.h"
#include <ECS/components/MultiMeshRenderer.h>
#include <ECS/components/Transform.h>
#include <ECS/components/Static.h>
#include "ECS/components/Light.h"
#include "ECS/TransformGraph.h"
#include "Engine.h"
#include "scene/Scene.h"
#include "objects/Camera.h"
#include "utils/MeshGeneration.h"
#include "utils/Math.h"
#include "render/debug/DebugDraw.h"

namespace ECS::systems
{
	namespace
	{
		inline quat LookAt(const vec3& direction, vec3 right, const vec3& up = vec3(0, 1, 0))
		{
			mat3 result;

			result[0] = right;
			result[1] = normalize(cross(-direction, result[0]));
			result[2] = normalize(cross(result[0], result[1]));

			return quat_cast(result);
		}
	}

	void ProjectileSystem::Process(ECS::Chunk* chunk)
	{
		ComponentFetcher<components::Transform> transform_fetcher(*chunk);
		ComponentFetcher<components::Projectile> projectile_fetcher(*chunk);
		auto dt = manager.GetStaticComponent<components::DeltaTime>();
		auto& camera_transform = Engine::Get()->GetScene()->GetCamera()->Transform();

		for (int i = 0; i < chunk->GetEntityCount(); i++)
		{
			auto* transform = transform_fetcher.GetComponent(i);
			auto* projectile = projectile_fetcher.GetComponent(i);

			projectile->prev_position = transform->position;
			const float distance = projectile->speed * dt->dt;
			transform->position += projectile->direction * distance;
			projectile->distance_travelled += distance;

			if (projectile->light_id)
			{
				auto light_transform = manager.GetComponent<components::Transform>(projectile->light_id);
				light_transform->position = transform->position;
			}

			if (projectile->distance_travelled > projectile->max_distance)
			{
				command_buffer.DestroyEntity(chunk->GetEntityID(i));
				if (projectile->light_id)
					command_buffer.DestroyEntity(projectile->light_id);
			}

			transform->rotation = LookAt(glm::normalize(camera_transform.position - transform->position), -projectile->direction, vec3(0, 1, 0));
		}
	}

}

namespace Projectile
{
	using namespace ECS;

	ProjectileManager::ProjectileManager(ECS::EntityManager& manager)
		: manager(manager)
		, projectile_system(manager)
	{
		material = Resources::MaterialResource::Handle(L"assets/top-down-shooter/characters/uetest/projectile.mat")->Get()->Clone();
		mesh = Mesh::Create(false);
		MeshGeneration::generateQuad(mesh.get(), vec2(1));
		mesh->createBuffer();
	}

	EntityID ProjectileManager::CreateProjectile(const Params& params)
	{
		auto projectile_material = params.material ? params.material : material;
		auto projectile_mesh = params.mesh ? params.mesh : mesh;

		EntityID id = manager.CreateEntity();

		{
			auto transform = manager.AddComponent<components::Transform>(id);
			transform->position = params.position;
			transform->scale = vec3(params.dimensions.x, params.dimensions.y, 1.0f);
			transform->rotation = quat();
		}

		{
			auto renderer = manager.AddComponent<components::MultiMeshRenderer>(id);
			auto multi_mesh = Resources::MultiMesh::Create(gsl::make_span(&projectile_mesh, 1));
			renderer->SetMultiMesh(multi_mesh);
			renderer->materials = render::MaterialList::Create();
			renderer->materials->push_back(projectile_material);
		}
		
		EntityID light_id = manager.CreateEntity();
		{
			{
				auto transform = manager.AddComponent<components::Transform>(light_id);
				transform->position = vec3(0);
			}

			{
				auto light = manager.AddComponent<components::Light>(light_id);
				light->color = vec3(10, 10, 10);
				light->radius = 2.0f;
			}
		}

		{
			auto projectile = manager.AddComponent<components::Projectile>(id);
			projectile->direction = glm::normalize(params.direction);
			projectile->speed = params.speed;
			projectile->max_distance = params.max_distance;
			projectile->prev_position = params.position;
			projectile->light_id = light_id;
		}

		return id;
	}

	void ProjectileManager::Update()
	{
		auto chunks = manager.GetChunkListsWithComponents<components::Projectile, components::MultiMeshRenderer>();
		projectile_system.ProcessChunks(chunks);
		projectile_system.GetCommandBuffer().Flush();
	}
}
