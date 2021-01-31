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
}

namespace game
{
	class Gameplay;
}

namespace Resources
{
	class TextureResource;
	class MultiMesh;
	class PhysCollider;
	class MaterialResource;
	class EntityResource;
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
	physx::PxVec3 GetGravity() override;
	void cleanup() override;

private:
	std::unique_ptr<ViewerCamera> camera;
	std::unique_ptr<ECS::components::DeltaTime> delta_time;
	std::unique_ptr<Vehicle::Utils::VehicleDataCache> vehicle_data_cache;
	std::unique_ptr<game::Gameplay> gameplay;

	ECS::EntityManager* manager = nullptr;
	ECS::TransformGraph* graph = nullptr;

	bool camera_control = false;
};
