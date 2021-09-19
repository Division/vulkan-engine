#include "Projectile.h"
#include <ECS/components/MultiMeshRenderer.h>
#include <ECS/components/Transform.h>
#include <ECS/components/Static.h>
#include "Engine.h"
#include "scene/Scene.h"
#include "objects/Camera.h"
#include "utils/MeshGeneration.h"

namespace ECS::systems
{
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
			if (projectile->distance_travelled > projectile->max_distance)
				command_buffer.DestroyEntity(chunk->GetEntityID(i));

			transform->LookAt(camera_transform.position);
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
			transform->rotation = glm::quat();
		}

		{
			auto renderer = manager.AddComponent<components::MultiMeshRenderer>(id);
			auto multi_mesh = Resources::MultiMesh::Create(gsl::make_span(&projectile_mesh, 1));
			renderer->SetMultiMesh(multi_mesh);
			renderer->materials = render::MaterialList::Create();
			renderer->materials->push_back(projectile_material);
		}

		{
			auto projectile = manager.AddComponent<components::Projectile>(id);
			projectile->direction = glm::normalize(params.direction);
			projectile->speed = params.speed;
			projectile->max_distance = params.max_distance;
			projectile->prev_position = params.position;
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
