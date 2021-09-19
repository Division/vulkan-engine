#pragma once

#include <vector>
#include "Engine.h"
#include "ecs/ECS.h"
#include "ecs/TransformGraph.h"
#include "ecs/systems/TransformSystem.h"
#include "ecs/systems/RendererSystem.h"
#include "ecs/components/Light.h"
#include "ecs/components/Physics.h"
#include "resources/ResourceCache.h"
#include "resources/MaterialResource.h"


class Mesh;
class Material;
class ViewerCamera;

namespace render
{
	class MaterialList;
}

namespace Vehicle::Utils
{
	class VehicleDataCache;
}

namespace ECS::components
{
	struct DeltaTime;
	struct CharacterController;
	struct Transform;
}

namespace game
{
	class Gameplay;
}

namespace Projectile
{
	class ProjectileManager;
}

namespace Resources
{
	class TextureResource;
	class MultiMesh;
	class PhysCollider;
	class MaterialResource;
	class EntityResource;
	class SkeletonResource;
	class SkeletalAnimationResource;
}

class Game : public IGame, public IGamePhysicsDelegate {
public:
	Game();
	~Game();
	void init() override;
	void update(float dt) override;
	void UpdatePhysics(float dt) override;
	InitParams GetInitParams() const override;
	IGamePhysicsDelegate* GetPhysicsDelegate() override;
	physx::PxSimulationFilterShader GetFilterShader() override;
	physx::PxVec3 GetGravity() override;
	void cleanup() override;

private:
	ECS::EntityID CreateLight(vec3 position, float radius, ECS::components::Light::Type type, vec3 color);
	ECS::EntityID CreatePlayer();

	void Shoot(ECS::components::Transform* player_transform, ECS::components::CharacterController* character_controller);
	void UpdatePlayer(float dt);
	void UpdateFollowCamera();
	std::optional<vec3> GetMouseTarget();

private:
	std::unique_ptr<ViewerCamera> camera;
	std::unique_ptr<Projectile::ProjectileManager> projectile_manager;

	ECS::EntityManager* manager = nullptr;
	ECS::TransformGraph* graph = nullptr;

	ECS::EntityID player_id;
	ECS::EntityID rifle_id;
	ECS::EntityID point_light_id;

	Resources::Handle<Resources::SkeletalAnimationResource> animation;
	double last_shoot_time = 0.0f;

	bool camera_control = false;
};
