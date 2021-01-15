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


class Mesh;
class FollowCamera;
class ModelBundle;
class MeshObject;
class PlayerController;
class Material;
class LightObject;
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
}

namespace Resources
{
	class TextureResource;
	class MultiMesh;
	class PhysCollider;
}

class Game : public IGame, public IGamePhysicsDelegate {
public:
	Game();
	~Game();
	void init() override;
	void update(float dt) override;
	void UpdatePhysics(float dt) override;
	IGamePhysicsDelegate* GetPhysicsDelegate() override;
	physx::PxSimulationFilterShader GetFilterShader() override;
	void cleanup() override;

	struct PhysicsInitializer;


private:
	ECS::EntityID CreateMeshEntity(vec3 position, ECS::EntityID parent, Mesh* mesh);
	ECS::EntityID CreateLightEntity(vec3 position, float radius, ECS::components::Light::Type type, vec3 color = vec3(1));
	ECS::EntityID CreateMultiMeshEntity(vec3 position, quat rotation, ECS::EntityID parent, const Resources::Handle<Resources::MultiMesh>& mesh, const Common::Handle<render::MaterialList>& materials);
	std::vector<ECS::EntityID> CreateStack(vec3 position, quat rotation, float half_extent, uint32_t count);
	physx::PxRigidActor* AddPhysics(ECS::EntityID entity, const PhysicsInitializer& init);

private:
	std::unique_ptr<ViewerCamera> camera;
	std::unique_ptr<ECS::components::DeltaTime> delta_time;
	std::unique_ptr<Vehicle::Utils::VehicleDataCache> vehicle_data_cache;
	std::shared_ptr<ModelBundle> player_model;

	ECS::EntityID entity1;
	ECS::EntityID entity2;
	ECS::EntityID plane;

	Common::Handle<Material> material_light_only;
	Common::Handle<Material> material_no_light;
	Common::Handle<Material> material_default;
	Common::Handle<Mesh> plane_mesh;
	Common::Handle<Mesh> sphere_mesh;
	Common::Handle<Mesh> box_mesh;
	Resources::Handle<ModelBundle> sphere_bundle;
	Resources::Handle<Resources::TextureResource> lama_tex;
	Resources::Handle<Resources::TextureResource> environment;
	Resources::Handle<Resources::MultiMesh> test_mesh;
	Resources::Handle<Resources::PhysCollider> player_collider;
	std::vector<Physics::Handle<physx::PxRigidStatic>> physics_static;
	ECS::EntityID test_mesh_entity;

	ECS::EntityManager* manager;
	ECS::TransformGraph* graph;

	std::vector<ECS::EntityID> temp_entities;

	bool camera_control = false;
};
